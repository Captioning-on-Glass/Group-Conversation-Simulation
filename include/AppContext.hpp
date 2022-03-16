//
// Created by quontas on 3/8/22.
//

#ifndef COG_GROUP_CONVO_CPP_APPCONTEXT_HPP
#define COG_GROUP_CONVO_CPP_APPCONTEXT_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_mutex.h>
#include <map>
#include <SDL2/SDL_ttf.h>
#include "captions.hpp"

struct AppContext {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_mutex *mutex;
    std::mutex *azimuth_mutex;
    std::deque<float> *azimuth_buffer;
    TTF_Font *smallest_font;
    TTF_Font *medium_font;
    TTF_Font *largest_font;
    const std::map<cog::Juror, std::pair<double, double>> *juror_positions;
    const std::map<cog::Juror, TTF_Font *> *juror_font_sizes;
    SDL_Surface *back_arrow;
    SDL_Surface *forward_arrow;
    SDL_Color *foreground_color;
    SDL_Color *background_color;
    CaptionModel *caption_model;
    int presentation_method;
    int n;
    int y;
    SDL_Rect display_rect;
    int window_width;
    int window_height;
};
#endif //COG_GROUP_CONVO_CPP_APPCONTEXT_HPP
