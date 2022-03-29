#include "presentation_methods.hpp"

void render_registered_captions(const AppContext *context) {
    const auto[caption_texture, juror] = context->caption_model->get_current_caption();
    // When we copy the caption texture to its destination, make sure that we don't do alpha blending. We just want to
    // replace the pixel values.
    SDL_SetTextureBlendMode(caption_texture, SDL_BLENDMODE_NONE);
    // We've previously identified where on the screen to place the captions underneath the jurors. Those are represented as percentages of the VLC surface fov_x_2/height
    auto[left_x_percent, left_y_percent] = context->juror_positions->at(juror);
    // Now we just re-hydrate those values with the current size of the VLC surface to get where the captions should be positioned.
    int text_x = left_x_percent * context->display_rect.w;
    int text_y = left_y_percent * context->display_rect.h;

    // Now we need to apply a mask for getting a good contrast ratio.
    // First, let's create an empty texture.
    auto texture = SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                     context->window_width, context->window_height);
    // Apparently SDL will ignore alpha values on SDL_RenderFillRect unless we set the texture's blend mode to SDL_BLENDMODE_BLEND.
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    // Now tell SDL that future "*Render*" commands should impact the texture we've created here, not the window. We'll
    // undo this at the end.
    SDL_SetRenderTarget(context->renderer, texture);
    // Now we change the color we'll be drawing with to the user-provided opacity.
    SDL_SetRenderDrawColor(context->renderer, 0, 0, 0, context->opacity);
    // And draw a rectangle on the entire surface (nullptr = "fill in everything on the texture") that's black, with the
    // given opacity.
    SDL_RenderFillRect(context->renderer, nullptr);

    // Because we're using textures (which are hardware-accelerated) instead of surfaces, we need to ask SDL how big our
    // caption texture is.
    int width = 0;
    int height = 0;
    SDL_QueryTexture(caption_texture, nullptr, nullptr, &width, &height);

    // Now, we're going to copy the entire caption texture
    auto src_rect = SDL_Rect{0, 0, width, height};

    // into its destination (which is underneath the appropriate juror).
    auto dst_rect = SDL_Rect{text_x, text_y, width, height};

    // Let's copy!
    SDL_RenderCopy(context->renderer, caption_texture, &src_rect, &dst_rect);

    // Since we're done constructing our texture, we need to change its blend mode back to BLENDMODE_BLEND, because
    // otherwise it will just overwrite the pixel values of the video when we copy our constructed onto the window.
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    // Now we tell SDL to apply all "*Render*" commands to the window, not a specific texture.
    SDL_SetRenderTarget(context->renderer, nullptr);

    // Copy our newly-constructed texture (with alpha blending enabled) onto the video frame!
    SDL_RenderCopy(context->renderer, texture, nullptr, nullptr);

    // Don't forget to free up the memory!
    SDL_DestroyTexture(texture);
}