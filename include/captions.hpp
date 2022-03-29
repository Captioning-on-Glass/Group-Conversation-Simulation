//
// Created by quontas on 3/7/22.
//

#ifndef COG_GROUP_CONVO_CPP_CAPTIONS_HPP
#define COG_GROUP_CONVO_CPP_CAPTIONS_HPP

#include <vector>
#include <netinet/in.h>
#include <SDL.h>
#include <SDL_surface.h>
#include "cog-flatbuffer-definitions/caption_message_generated.h"
#include "nlohmann/json.hpp"


class CaptionModel {
private:
    size_t idx = 0;
    std::vector<std::pair<SDL_Texture *, cog::Juror>> captions;
public:
    CaptionModel(SDL_Renderer *renderer, const std::string &path_to_bitmaps, const std::vector<cog::Juror> &speaker_ids,
                 int blur_level);

    ~CaptionModel();

    void increment();

    std::pair<SDL_Texture *, cog::Juror> get_current_caption();
};

cog::Juror juror_from_string(const std::string &juror_str);

void
start_caption_stream(const bool *started, nlohmann::json *caption_json, CaptionModel *model);

#endif //COG_GROUP_CONVO_CPP_CAPTIONS_HPP
