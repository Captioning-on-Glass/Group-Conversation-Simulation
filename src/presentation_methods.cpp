#include <iostream>
#include "presentation_methods.hpp"
#include "orientation.hpp"

std::optional<SDL_Rect> rectangle_intersection(const SDL_Rect *a, const SDL_Rect *b) {
    int intersection_tl_x = std::max(a->x, b->x);
    int intersection_tl_y = std::max(a->y, b->y);
    int intersection_br_x = std::min(a->x + a->w, b->x + b->w);
    int intersection_br_y = std::min(a->y + a->h, b->y + b->h);
    if (intersection_tl_x >= intersection_br_x || intersection_tl_y >= intersection_br_y) {
        return std::nullopt;
    }
    int width = intersection_br_x - intersection_tl_x;
    int height = intersection_br_y - intersection_tl_y;
    return SDL_Rect{intersection_tl_x, intersection_tl_y, width, height};
}


void render_surface_as_texture(SDL_Renderer *renderer, SDL_Surface *surface, SDL_Rect *source_rect,
                               SDL_Rect *destination_rect) {
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderCopy(renderer, texture, source_rect, destination_rect);
    SDL_DestroyTexture(texture);
}

std::tuple<int, int> render_text(SDL_Renderer *renderer, TTF_Font *font, const std::string &text, int x, int y,
                                 const SDL_Color *foreground_color, const SDL_Color *background_color)
{

    auto text_surface = TTF_RenderText_Shaded_Wrapped(font,
                                                      text.c_str(),
                                                      *foreground_color,
                                                      *background_color,
                                                      WRAP_LENGTH);
    int w = text_surface->w;
    int h = text_surface->h;
    auto destination_rect = SDL_Rect{x, y, w, h};
    render_surface_as_texture(renderer, text_surface, nullptr, &destination_rect);
    SDL_FreeSurface(text_surface);

    std::cout<<"Width: " << w << "\n";
    return std::make_tuple(w, h);
}


void render_nonregistered_captions(const AppContext *context) {
    auto[juror, text] = context->caption_model->get_current_text();
    if (text.empty()) return;

    auto left_x = filtered_azimuth(context->azimuth_buffer, context->azimuth_mutex);
    const auto adjusted_x = angle_to_pixel_position(left_x) + context->window_width / 3;
    render_text(context->renderer, context->medium_font, text, adjusted_x, context->y,
                context->foreground_color,
                context->background_color);
}


void render_nonregistered_captions_with_indicators(const AppContext *context) {

    auto[juror, text] = context->caption_model->get_current_text();
    if ( text.empty() ) return;

//    auto left_x = filtered_azimuth(context->azimuth_buffer, context->azimuth_mutex);

    auto left_x = exponential_filtered_azimuth(context->azimuth_buffer, context->azimuth_mutex);

    const auto adjusted_x = pixel_mapped(left_x, context);

    const auto[text_width, text_height] =
    render_text(context->renderer,
                context->medium_font,
                text,
                adjusted_x,
                context->y,
                context->foreground_color,
                context->background_color);

    bool should_show_forward_arrow = false;
    bool should_show_back_arrow = false;
    const auto[left, right] = context->juror_intervals->at(juror);


    if ((adjusted_x + text_width / 2) < left) {
        should_show_forward_arrow = true;
    } else if ((adjusted_x + text_width / 2) > right) {
        should_show_back_arrow = true;
    }
    int arrow_x = adjusted_x;
    SDL_Surface *arrow_surface = nullptr;
    if (should_show_back_arrow) {
        arrow_surface = context->back_arrow;
        arrow_x -= arrow_surface->w;
    } else if (should_show_forward_arrow) {
        arrow_surface = context->forward_arrow;
        arrow_x += text_width;
    }

    if (!(should_show_back_arrow || should_show_forward_arrow)) {
        return;
    }
    auto destination_rect = SDL_Rect{arrow_x,
                                     context->y,
                                     arrow_surface->w,
                                     arrow_surface->h};

    render_surface_as_texture(context->renderer,
                              arrow_surface,
                              nullptr,
                              &destination_rect);
}

void render_registered_captions(const AppContext *context) {
    const auto[juror, text] = context->caption_model->get_current_text();
    if (text.empty()) {
        return;
    }
    // We've previously identified where on the screen to place the captions u nderneath the jurors. Those are represented as percentages of the VLC surface fov_x_2/height
    auto[left_x_percent, left_y_percent] = context->juror_positions->at(juror);
    // Now we just re-hydrate those values with the current size of the VLC surface to get where the captions should be positioned.
    int text_x = left_x_percent * context->display_rect.w;
    int text_y = left_y_percent * context->display_rect.h;
    // Retrieve the font to be used for the current juror
    auto font = context->juror_font_sizes->at(juror);
    // And let's create a surface of the text we've been given.
    auto text_surface = TTF_RenderText_Shaded_Wrapped(font, text.c_str(), *context->foreground_color,
                                                      *context->background_color,
                                                      WRAP_LENGTH);
//    SDL_Rect rect = {text_x, text_y, text_surface->w, text_surface->h};
//    render_surface_as_texture(context->renderer, text_surface, nullptr, &rect);
//    return;

    // Now, here's where we do our clipping behavior.
    // The general idea is as follows:
    //
    // The text surface has a width and height, and we know the text_x and text_y of where we're going to draw the
    // caption (assuming no clipping at all).
    const auto surface_rect = SDL_Rect{text_x, text_y, text_surface->w, text_surface->h};

    // We also have a pre-defined field-of-view (FOV), which is how much the person would be able to see if they were
    // wearing a realistic HWD.
    auto azimuth = filtered_azimuth(context->azimuth_buffer, context->azimuth_mutex);
    const auto half_fov_in_radians = to_radians(context->half_fov);

    // We can calculate how much of the window fov_x_2 the FOV covers with some trig...
    const auto fov_x = angle_to_pixel_position(azimuth) - angle_to_pixel_position(to_radians(context->half_fov)) +
                       context->window_width / 3;
    const auto fov_x_2 = angle_to_pixel_position(azimuth) + angle_to_pixel_position(to_radians(context->half_fov)) +
                         context->window_width / 3;
    auto l = std::min(fov_x, fov_x_2);
    auto r = std::max(fov_x, fov_x_2);
    const auto fov_region = SDL_Rect{l, 0, r - l, context->window_height};

//    SDL_SetRenderDrawBlendMode(context->renderer, SDL_BLENDMODE_BLEND);
//    SDL_SetRenderDrawColor(context->renderer, 0, 0, 0, 128);
//    SDL_RenderFillRect(context->renderer, &fov_region);
//    SDL_SetRenderDrawColor(context->renderer, 255, 0, 0, 255);
//    const auto azimuth_x = angle_to_pixel_position(azimuth) + (context->window_width);
//    SDL_RenderDrawLine(context->renderer, azimuth_x, 0, azimuth_x, context->window_height);

    // and then find the intersection between the FOV region (which extends from the top to the bottom of the window, to
    // keep things easy) and the text surface rectangle, which should give us a rectangle indicating what part of the
    // caption should be rendered.
    auto intersection = rectangle_intersection(&surface_rect, &fov_region);
    // If they don't intersect at all, there's nothing to render, we stop here.
    int arrow_x = (l + r) / 2;
    SDL_Surface *arrow_surface = nullptr;
    if (!intersection.has_value()) {
        if(l > text_x) {
            arrow_surface = context->back_arrow;
        } else{
            arrow_surface = context->forward_arrow;
        }
        auto destination_rect = SDL_Rect{arrow_x, context->y - 400 , arrow_surface->w, arrow_surface->h};
        render_surface_as_texture(context->renderer, arrow_surface, nullptr, &destination_rect);
        SDL_FreeSurface(text_surface);
        return;
    }
    SDL_Rect intersection_rect = intersection.value();

    // One thing to note: our intersection rectangle could be located anywhere on the screen
    // (0 <= intersection_x <= WINDOW_WIDTH) and (0 <= intersection_y <= WINDOW_HEIGHT)
    // But that's not what we want! We want to know how much of the TEXT SURFACE we want to render.
    // So let's calculate how far intersection_rect.x is from text_x, and how far intersection_rect.y is from text_y
    SDL_Rect text_surface_clip_region = {
            intersection_rect.x - text_x, // This won't ever be negative, because intersection_x >= text_x always
            intersection_rect.y - text_y, // Same here
            intersection_rect.w, // And this is how much of the text surface we want to clip
            intersection_rect.h
    };
    // Now, last thing: We now know what part of the text surface we need to clip, but we need to RENDER it at the
    // intersection between the FOV and the caption rectangle.
    // We're going to copy the pixels from the region outlined by text_surface_clip_region on text_surface to the
    // pixels outlined by intersection_rect. That should give us the clipped caption on the display!
    render_surface_as_texture(context->renderer, text_surface, &text_surface_clip_region, &intersection_rect);
    SDL_FreeSurface(text_surface);
}