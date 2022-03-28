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

std::tuple<int, int>
render_text(SDL_Renderer *renderer, TTF_Font *font, const std::string &text, int x, int y,
            const SDL_Color *foreground_color, const SDL_Color *background_color) {
    auto text_surface = TTF_RenderText_Shaded_Wrapped(font, text.c_str(), *foreground_color,
                                                      *background_color,
                                                      WRAP_LENGTH);
    int w = text_surface->w;
    int h = text_surface->h;
    auto destination_rect = SDL_Rect{x, y, w, h};
    render_surface_as_texture(renderer, text_surface, nullptr, &destination_rect);
    SDL_FreeSurface(text_surface);
    return std::make_tuple(w, h);
}


void render_nonregistered_captions(const AppContext *context) {
    auto left_x = calculate_display_x_from_orientation(context);
    auto[caption_texture, juror] = context->caption_model->get_current_caption();
    int width = 0;
    int height = 0;
    SDL_QueryTexture(caption_texture, nullptr, nullptr, &width, &height);
    auto destination_rect = SDL_Rect{static_cast<int>(left_x), context->y, width, height};
    SDL_RenderCopy(context->renderer, caption_texture, nullptr, &destination_rect);
}

void render_registered_captions(const AppContext *context) {
    const auto[caption_texture, juror] = context->caption_model->get_current_caption();
    SDL_SetTextureBlendMode(caption_texture, SDL_BLENDMODE_NONE);
    // We've previously identified where on the screen to place the captions underneath the jurors. Those are represented as percentages of the VLC surface fov_x_2/height
    auto[left_x_percent, left_y_percent] = context->juror_positions->at(juror);
    // Now we just re-hydrate those values with the current size of the VLC surface to get where the captions should be positioned.
    int text_x = left_x_percent * context->display_rect.w;
    int text_y = left_y_percent * context->display_rect.h;
    auto texture = SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                     context->window_width, context->window_height);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawBlendMode(context->renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderTarget(context->renderer, texture);
    SDL_SetRenderDrawColor(context->renderer, 0, 0, 0, 192);
    SDL_RenderFillRect(context->renderer, nullptr);
    int width = 0;
    int height = 0;
    SDL_QueryTexture(caption_texture, nullptr, nullptr, &width, &height);
    auto src_rect = SDL_Rect{0, 0, width, height};
    auto dst_rect = SDL_Rect{text_x, text_y, width, height};
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(context->renderer, caption_texture, &src_rect, &dst_rect);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(context->renderer, nullptr);
    SDL_RenderCopy(context->renderer, texture, nullptr, nullptr);
}