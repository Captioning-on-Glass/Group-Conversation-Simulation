//
// Created by quontas on 3/8/22.
//

#ifndef COG_GROUP_CONVO_CPP_APPCONTEXT_HPP
#define COG_GROUP_CONVO_CPP_APPCONTEXT_HPP

#include <map>
#include <SDL.h>
#include <SDL_mutex.h>
#include <SDL_ttf.h>
#include "captions.hpp"

struct AppContext {
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_mutex *mutex;
    const std::map<cog::Juror, std::pair<double, double>> *juror_positions;
    int opacity;
    CaptionModel *caption_model;
    int presentation_method;
    int n;
    int y;
    SDL_Rect display_rect;
    int window_width;
    int window_height;
};
#endif //COG_GROUP_CONVO_CPP_APPCONTEXT_HPP
