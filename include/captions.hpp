//
// Created by quontas on 3/7/22.
//

#ifndef COG_GROUP_CONVO_CPP_CAPTIONS_HPP
#define COG_GROUP_CONVO_CPP_CAPTIONS_HPP

#include <vector>
#include <netinet/in.h>
#include "cog-flatbuffer-definitions/caption_message_generated.h"
#include "nlohmann/json.hpp"


class CaptionModel {
private:
    std::vector<std::pair<cog::Juror, std::string>> spoken_so_far;
    std::mutex text_mutex;
    const static int LINE_LENGTH = 30;

    static std::string wrap(const std::string &text, int line_length);

public:
    explicit CaptionModel() = default;

    void add_word(const std::string &new_word, cog::Juror speaker);

    std::pair<cog::Juror, std::string> get_current_text(int line_length = LINE_LENGTH);
};

cog::Juror juror_from_string(const std::string &juror_str);

void
start_caption_stream(const bool *started, int socket, sockaddr_in *client_address, nlohmann::json *caption_json,
                     CaptionModel *model, const int presentation_method);

#endif //COG_GROUP_CONVO_CPP_CAPTIONS_HPP
