#include <cstdio>
#include <iostream>
#include "experiment_setup.hpp"
#include "presentation_methods.hpp"
#include "nlohmann/json.hpp"
#include "captions.hpp"
#include "orientation.hpp"
#include <thread>
#include <fstream>
#include <cstdlib>
#include <vlc/vlc.h>

#include <SDL.h>
#include <SDL_mutex.h>
#include <SDL_image.h>

#define WINDOW_TITLE "Four Angry Men"

#define REGISTERED_GRAPHICS 1

#define WINDOW_OFFSET_X 83 // ASSUMING 3840x2160 DISPLAY
#define WINDOW_OFFSET_Y 292

constexpr auto PRESENTATION_METHOD = REGISTERED_GRAPHICS;
constexpr auto OPACITY = 0;

/**
 * This function is called prior to VLC rendering a video frame.
 * What we do here is lock our mutex (preventing other threads from touching the texture), and lock the texture from being
 * modified by other threads. This allows VLC to render to the texture peacefully, without data races.
 * @param data A pointer to data that would be useful for whatever we want to do in this function (in this case, AppContext, for mutex/texture access)
 * @param p_pixels An array of pixels representing the image, stored as concatenated rows
 * @return nullptr.
 */
static void *lock(void *data, void **p_pixels) {
    const auto app_context = static_cast<AppContext *>(data);
    int pitch;
    SDL_LockMutex(app_context->mutex);
    SDL_LockTexture(app_context->texture, nullptr, p_pixels, &pitch);

    return nullptr; // Picture identifier, not needed here.
}

/**
 * This function is called after VLC renders a video frame. Once VLC is done writing a frame, we want to immediately overlay the captions on top of the frame, according to the presentation method provided, and render the frame. Once rendered, that caption, we unlock the mutex and texture for other threads to access.
 * @param data A pointer to data that would be useful for whatever we want to do in this function (in this case, AppContext, for presentation method/mutex/texture access)
 * @param id Honestly? Not really sure what this parameter is, but I haven't needed it. It's here to comply with the function signature expected by VLC.
 * @param p_pixels An array of pixels representing the image, stored as concatenated rows
 */
static void unlock(void *data, [[maybe_unused]] void *id, [[maybe_unused]] void *const *p_pixels) {

    const auto app_context = static_cast<AppContext *>(data);

    // Based on the presentation method selected by the researcher, we want to render captions in different ways.
    switch (app_context->presentation_method) {
        case REGISTERED_GRAPHICS:
            // Registered graphics remain stationary in space
            render_registered_captions(app_context);
            break;
        default:
            std::cerr << "Unknown method received: " << app_context->presentation_method << std::endl;
            exit(EXIT_FAILURE);
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
    auto *app_context = static_cast<AppContext *>(data);

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
    blur_level // How much blur will we be applying to our captions?
    ] = parse_arguments(argc, argv);

    const auto presentation_method = PRESENTATION_METHOD;

    std::cout << "Using presentation method: " << presentation_method << std::endl;
    std::cout << "Playing video section: " << video_section << std::endl;
    std::cout << "Using blur level: " << blur_level << std::endl;

    // Let's start building our application context. This is basically a struct that stores pointers to
    // important mutexes, buffers, and variables.

    // Hard-coded positions of where captions should be rendered on the video.
    const std::map<cog::Juror, std::pair<double, double>> juror_positions{
            {cog::Juror_JurorA,      {1050.f / 1920.f, 550.f / 1080.f}},
            {cog::Juror_JurorB,      {675.f / 1920.f,  550.f / 1080.f}},
            {cog::Juror_JurorC,      {197.f / 1920.f,  650.f / 1080.f}},
            {cog::Juror_JuryForeman, {1250.f / 1920.f, 600.f / 1080.f}}
    };
    struct AppContext app_context{};
    app_context.opacity = OPACITY;
    app_context.presentation_method = presentation_method;
    app_context.juror_positions = &juror_positions;
    app_context.window_width = SCREEN_PIXEL_WIDTH;
    app_context.window_height = SCREEN_PIXEL_HEIGHT;
    // For non-registered captions, render them at 75% of the window's height.
    app_context.y = app_context.window_height * 0.75;

    // Let's initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    // Lastly, let's initialize SDL_image, which will load images for us.
    auto flags = IMG_INIT_PNG;
    if ((IMG_Init(flags) & flags) != flags) {
        printf("IMG_Init: Failed to init required png support!\n");
        printf("IMG_Init: %s\n", IMG_GetError());
    }

    auto window_pos_x = WINDOW_OFFSET_X;
    auto window_pos_y = WINDOW_OFFSET_Y;
    auto displays = SDL_GetNumVideoDisplays();
    if (displays > 1) {
        SDL_Rect second_display_bounds{};
        SDL_GetDisplayBounds(1, &second_display_bounds);
        window_pos_x = second_display_bounds.x + WINDOW_OFFSET_X;
        window_pos_y = second_display_bounds.y + WINDOW_OFFSET_Y;
    }
    // Create the window that we'll use
    auto window = SDL_CreateWindow(WINDOW_TITLE, 0, 0,
                                   app_context.window_width,
                                   app_context.window_height, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowPosition(window, window_pos_x, window_pos_y);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "Linear");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    app_context.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
                                                          SDL_RENDERER_TARGETTEXTURE);
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
    // If you don't have this variable set you must have plugins directory
    // with the executable or libvlc_new() will not work!
    printf("VLC_PLUGIN_PATH=%s\n", getenv("VLC_PLUGIN_PATH"));

    // Initialise libVLC.
    libvlc = libvlc_new(vlc_argc, vlc_argv);
    if (nullptr == libvlc) {
        printf("LibVLC initialization failure. If on MacOS, make sure that VLC_PLUGIN_PATH is set to the path of the PARENT of the VLC plugin folder");
        return EXIT_FAILURE;
    }
    std::ostringstream os;

    nlohmann::json json;
    os << "resources/captions/merged_captions." << video_section << ".json";
    std::string captions_path = os.str();
    std::cout << "Captions path = " << captions_path << std::endl;
    std::ifstream captions_file(captions_path.c_str());
    captions_file >> json;
    std::vector<cog::Juror> speaker_ids;
    for (const auto &entry: json) {
        const auto speaker_id_str = entry["speaker_id"].get<std::string>();
        auto speaker_id = juror_from_string(speaker_id_str);
        speaker_ids.emplace_back(speaker_id);
    }
    std::string path_to_bmps = "resources/bmp/" + std::to_string(video_section);
    std::cout << "Path to bmps = " << path_to_bmps << std::endl;
    auto caption_model = CaptionModel(app_context.renderer, path_to_bmps, speaker_ids,
                                      blur_level);
    app_context.caption_model = &caption_model;

    os.str("");
    os.clear();
    // Let's load the video that we're going to play on VLC
    os << "resources/videos/main." << video_section << ".mp4";
    std::string video_path = os.str();
    m = libvlc_media_new_path(libvlc, video_path.c_str());
    mp = libvlc_media_player_new_from_media(m);
    libvlc_media_release(m);
    libvlc_video_set_callbacks(mp, lock, unlock, display, &app_context);
    libvlc_video_set_format(mp, "RV16", app_context.window_width, app_context.window_height,
                            app_context.window_width * 2);

    bool started = false;

    SDL_Event event;
    bool done = false;
    int action = 0;
    // Main loop.
    std::thread play_captions_thread(start_caption_stream, &started,
                                     &json,
                                     &caption_model);
    SDL_RenderPresent(app_context.renderer);
    while (!done) {
        action = 0;

        // Keys: enter (fullscreen), space (pause), escape (quit).
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    done = true;
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
                done = true;
                break;
            case SDLK_SPACE:
                if (!started) {
                    started = true;
                    libvlc_media_player_play(mp);
                }
                break;
            default:
                break;
        }

        SDL_Delay(1000 / 10);
    }
    SDL_DestroyMutex(app_context.mutex);
    SDL_DestroyRenderer(app_context.renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}