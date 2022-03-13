#include <cstdio>
#include <iostream>
#include "experiment_setup.hpp"
#include "presentation_methods.hpp"
#include "nlohmann/json.hpp"
#include "captions.hpp"
#include <thread>
#include <fstream>
#include <cstdlib>
#include <vlc/vlc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mutex.h>
#include <SDL_image.h>

#define PORT 65432

#define WIDTH 3840 // How wide do we want the created window to be?
#define HEIGHT 2160 // How tall do we want the created window to be?

#define FONT_SIZE_SMALL 12
#define FONT_SIZE_MEDIUM 16
#define FONT_SIZE_LARGE 18

#define WINDOW_TITLE "Four Angry Men"

#define REGISTERED_GRAPHICS 1
#define NONREGISTERED_GRAPHICS 2
#define NONREGISTERED_GRAPHICS_WITH_ARROWS 3


/**
 * This function is called prior to VLC rendering a video frame.
 * What we do here is lock our mutex (preventing other threads from touching the texture), and lock the texture from being
 * modified by other threads. This allows VLC to render to the texture peacefully, without data races.
 * @param data A pointer to data that would be useful for whatever we want to do in this function (in this case, AppContext, for mutex/texture access)
 * @param p_pixels An array of pixels representing the image, stored as concatenated rows
 * @return nullptr.
 */
static void *lock(void *data, void **p_pixels) {
    auto *c = (AppContext *) data;
    int pitch;
    SDL_LockMutex(c->mutex);
    SDL_LockTexture(c->texture, nullptr, p_pixels, &pitch);

    return nullptr; // Picture identifier, not needed here.
}

/**
 * This function is called after VLC renders a video frame. Once VLC is done writing a frame, we want to immediately overlay the captions on top of the frame, according to the presentation method provided, and render the frame. Once rendered, that caption, we unlock the mutex and texture for other threads to access.
 * @param data A pointer to data that would be useful for whatever we want to do in this function (in this case, AppContext, for presentation method/mutex/texture access)
 * @param id Honestly? Not really sure what this parameter is, but I haven't needed it. It's here to comply with the function signature expected by VLC.
 * @param p_pixels An array of pixels representing the image, stored as concatenated rows
 */
static void unlock(void *data, [[maybe_unused]] void *id, [[maybe_unused]] void *const *p_pixels) {

    const auto *app_context = (AppContext *) data;

    // Based on the presentation method selected by the researcher, we want to render captions in different ways.
    switch (app_context->presentation_method) {
        case REGISTERED_GRAPHICS:
            // Registered graphics remain stationary in space
            render_registered_captions(app_context);
            break;
        case NONREGISTERED_GRAPHICS:
            // Non-registered graphics follow the user's head orientation around the screen
            render_nonregistered_captions(app_context);
            break;
        case NONREGISTERED_GRAPHICS_WITH_ARROWS:
            render_nonregistered_captions_with_indicators(app_context);
            break;
        default:
            std::cout << "Unknown method received: " << app_context->presentation_method << std::endl;
            break;
    }
    SDL_RenderPresent(app_context->renderer);
    SDL_UnlockTexture(app_context->texture);
    SDL_UnlockMutex(app_context->mutex);
}

/**
 * This function is called when VLC wants to render a frame. We create a blank rectangle texture,
 * which VLC will render to outside of this program.
 * @param data A pointer to data that would be useful for whatever we want to do in this function (in this case, AppContext, for presentation method/mutex/texture access)
 * @param id Honestly? Not really sure what this parameter is, but I haven't needed it. It's here to comply with the function signature expected by VLC.
 */
static void display(void *data, void *id) {

    auto *app_context = (AppContext *) data;

    app_context->display_rect.x = 0;
    app_context->display_rect.y = 0;
    app_context->display_rect.w = app_context->window_width;
    app_context->display_rect.h = app_context->window_height;
    SDL_SetRenderDrawColor(app_context->renderer, 0, 0, 0, 255);
    SDL_RenderClear(app_context->renderer);
    SDL_RenderCopy(app_context->renderer, app_context->texture, nullptr, &app_context->display_rect);
}

int main(int argc, char *argv[]) {
    // Get command-line arguments, which will be used for configuring how captions are rendered.
    const auto
    [
    video_section, // Which video section will we be rendering?
    presentation_method, // How will we be presenting captions?
    fg, // What color will the text be? RGBA format
    bg, // What color will the background behind the text be? RGBA format
    path_to_font, // Where's the smallest_font located?
    font_size // How big will the smallest_font be?
    ] = parse_arguments(argc, argv);

    std::cout << "Using presentation method: " << presentation_method << std::endl;
    std::cout << "Playing video section: " << video_section << std::endl;

    // Print the address of this server, and which presentation method we're going to be using.
    // This QR code will be scanned by the HWD so that it can connect to our server.
    print_connection_qr(presentation_method, PORT);
    // Now, wait for the connection.
    auto[socket, cliaddr] = connect_to_client(PORT);

    // Let's start building our application context. This is basically a struct that stores pointers to
    // important mutexes, buffers, and variables.
    const std::map<cog::Juror, std::pair<double, double>> juror_positions{
            {cog::Juror_JurorA,      {838.f / 1920.f, 431.f / 1080.f}},
            {cog::Juror_JurorB,      {512.f / 1920.f, 452.f / 1080.f}},
            {cog::Juror_JurorC,      {197.f / 1920.f, 528.f / 1080.f}},
            {cog::Juror_JuryForeman, {963.f / 1920.f, 532.f / 1080.f}}
    };
    struct AppContext app_context{};
    app_context.presentation_method = presentation_method;
    app_context.juror_positions = &juror_positions;
    app_context.window_width = WIDTH;
    app_context.window_height = HEIGHT;
    app_context.y = app_context.window_height * 0.75;


    // Let's initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return 1;
    }
    // And initialize SDL_ttf, which will render text on our video frames.
    if (TTF_Init() == -1) {
        printf("[ERROR] TTF_Init() Failed with: %s\n", TTF_GetError());
        exit(2);
    }

    auto flags = IMG_INIT_PNG;
    int initialized = IMG_Init(flags);
    if ((initialized & flags) != flags) {
        printf("IMG_Init: Failed to init required png support!\n");
        printf("IMG_Init: %s\n", IMG_GetError());
    }

    std::string back_arrow_path = "resources/images/arrow_back.png";
    std::string forward_arrow_path = "resources/images/arrow_forward.png";
    SDL_Surface *back_arrow = IMG_Load(back_arrow_path.c_str());
    if (!back_arrow) {
        printf("IMG_Load: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_Surface *forward_arrow = IMG_Load(forward_arrow_path.c_str());
    if (!forward_arrow) {
        printf("IMG_Load: %s\n", IMG_GetError());
        exit(EXIT_FAILURE);
    }
    app_context.back_arrow = back_arrow;
    app_context.forward_arrow = forward_arrow;

    TTF_Font *smallest_font = TTF_OpenFont(path_to_font.c_str(), FONT_SIZE_SMALL);
    TTF_Font *medium_font = TTF_OpenFont(path_to_font.c_str(), FONT_SIZE_MEDIUM);
    TTF_Font *largest_font = TTF_OpenFont(path_to_font.c_str(), FONT_SIZE_LARGE);
    app_context.smallest_font = smallest_font;
    app_context.medium_font = medium_font;
    app_context.largest_font = largest_font;
    const std::map<cog::Juror, TTF_Font *> juror_font_sizes{
            {cog::Juror_JurorA,      smallest_font},
            {cog::Juror_JurorB,      smallest_font},
            {cog::Juror_JuryForeman, largest_font},
            {cog::Juror_JurorC,      largest_font}
    };
    app_context.juror_font_sizes = &juror_font_sizes;

    SDL_Color foreground_color = {fg.at(0), fg.at(1), fg.at(2), fg.at(3)};
    SDL_Color background_color = {bg.at(0), bg.at(1), bg.at(2), bg.at(3)};
    app_context.foreground_color = &foreground_color;
    app_context.background_color = &background_color;

    auto window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   app_context.window_width,
                                   app_context.window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return 1;
    }
//    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "Linear");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    app_context.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    app_context.texture = SDL_CreateTexture(app_context.renderer, SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING,
                                            app_context.window_width, app_context.window_height);
    if (!app_context.texture) {
        fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
    }
    app_context.mutex = SDL_CreateMutex();


    // Now that we've configured our app context, let's get ready to boot up VLC.
    libvlc_instance_t *libvlc;
    libvlc_media_t *m;
    libvlc_media_player_t *mp;
    char const *vlc_argv[] = {
            "--no-audio", // Don't play audio.
            "--no-xlib", // Don't use Xlib.
    };
    int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
    int done = 0, action, pause = 0;
    // If you don't have this variable set you must have plugins directory
    // with the executable or libvlc_new() will not work!
    printf("VLC_PLUGIN_PATH=%s\n", getenv("VLC_PLUGIN_PATH"));

    // Initialise libVLC.
    libvlc = libvlc_new(vlc_argc, vlc_argv);
    if (nullptr == libvlc) {
        printf("LibVLC initialization failure. If on MacOS, make sure that VLC_PLUGIN_PATH is set to the path of the PARENT of the VLC plugin folder");
        return EXIT_FAILURE;
    }

    // Let's load the video that we're going to play on VLC
    std::ostringstream os;
    os << "resources/videos/main.mp4";
    std::string video_path = os.str();
    m = libvlc_media_new_path(libvlc, video_path.c_str());
    mp = libvlc_media_player_new_from_media(m);
    libvlc_media_release(m);
    libvlc_video_set_callbacks(mp, lock, unlock, display, &app_context);
    libvlc_video_set_format(mp, "RV16", app_context.window_width, app_context.window_height,
                            app_context.window_width * 2);

    std::mutex azimuth_mutex;
    app_context.azimuth_mutex = &azimuth_mutex;
    std::deque<float> azimuth_buffer;
    app_context.azimuth_buffer = &azimuth_buffer;
    std::mutex socket_mutex;
    std::thread read_orientation_thread(read_orientation, socket, cliaddr, &socket_mutex, &azimuth_mutex,
                                        &azimuth_buffer);

    nlohmann::json json;
    os.str("");
    os.clear();
    os << "resources/captions/captions_main.json";
    std::string captions_path = os.str();
    std::cout << "Captions path = " << captions_path << std::endl;
    std::ifstream captions_file(captions_path.c_str());
    captions_file >> json;
    auto caption_model = CaptionModel();
    app_context.caption_model = &caption_model;

    // Wait for data to start getting transmitted from the phone
    // before we start playing our video on VLC and rendering captions.
//    while (azimuth_buffer.size() < MOVING_AVG_SIZE) {
//    }
    libvlc_media_player_play(mp);
    std::thread play_captions_thread(play_captions, &json, &caption_model);
    SDL_Event event;
    int x, y;
    // Main loop.
    while (!done) {
        SDL_GetMouseState(&x, &y);
        SDL_Log("Mouse cursor is at %d, %d", x, y);
        action = 0;

        // Keys: enter (fullscreen), space (pause), escape (quit).
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    done = 1;
                    break;
                case SDL_KEYDOWN:
                    action = event.key.keysym.sym;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        SDL_RenderSetViewport(app_context.renderer, nullptr);
                        app_context.display_rect.w = app_context.window_width = event.window.data1;
                        app_context.display_rect.h = app_context.window_height = event.window.data2;
                        app_context.y = app_context.window_height * 0.75;
                    }
                    break;
            }
        }

        switch (action) {
            case SDLK_ESCAPE:
            case SDLK_q:
                done = 1;
                break;
            case SDLK_DOWN:
                app_context.y += 100;
                break;
            case SDLK_UP:
                app_context.y -= 100;
                break;
            default:
                break;
        }

        SDL_Delay(1000 / 10);
    }
    TTF_CloseFont(smallest_font);
    SDL_DestroyMutex(app_context.mutex);
    SDL_DestroyRenderer(app_context.renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}