#ifndef COG_GROUP_CONVO_CPP_PRESENTATION_METHODS_HPP
#define COG_GROUP_CONVO_CPP_PRESENTATION_METHODS_HPP

#include <SDL.h>
#include <optional>
#include <SDL_ttf.h>
#include "AppContext.hpp"

constexpr int WRAP_LENGTH = 0;// this is pixel width of the caption TODO laird look at this
//constexpr int HALF_FOV = 40;

/**
 * Return the intersection between two SDL_Rects as another SDL_Rect. If there is no intersection, return nullopt
 * @param a
 * @param b
 * @return An SDL_Rect if there is an intersection, otherwise nullopt.
 */
std::optional<SDL_Rect> rectangle_intersection(const SDL_Rect *a, const SDL_Rect *b);


/**
 * Renders the provided surface as a texture on the given renderer, using the position, width, and height provided.
 * @param renderer A pointer to an SDL_Renderer, which the surface will be rendered on
 * @param surface A pointer to the surface to be rendered
 * @param destination_rect The destination rectangle on which to render the surface
 * @param source_rect A rectangle outlining what section of the surface we want to render.
 */
void render_surface_as_texture(SDL_Renderer *renderer, SDL_Surface *surface, SDL_Rect *source_rect,
                               SDL_Rect *destination_rect);

/**
 * Renders the given text on the renderer using the position, colors, and font provided.
 * Returns the width and height of the texture rendered.
 * @param renderer A pointer to an SDL_Renderer, to which the text will be rendered
 * @param font A pointer to a TTF_Font, which will be used to display the text.
 * @param text The actual string to be displayed
 * @param x The x location at which to display the string
 * @param y The y location at which to display the string
 * @param foreground_color The color of the text
 * @param background_color The color of the background
 * @return
 */
std::tuple<int, int>
render_text(SDL_Renderer *renderer, TTF_Font *font, const std::string &text, int x, int y,
            const SDL_Color *foreground_color, const SDL_Color *background_color);


void render_nonregistered_captions(const AppContext *context);

/**
 * Renders non-registered captions (captions that remain at a fixed location in the user's field-of-view) using the given app context.
 * @param context
 */
void render_nonregistered_captions_with_indicators(const AppContext *context);

/**
 * Renders registered captions (which remain fixed in space, pinned to the body of the person speaking)
 * using the given app context.
 * @param context
 */
void render_registered_captions(const AppContext *context);

#endif //COG_GROUP_CONVO_CPP_PRESENTATION_METHODS_HPP
