//
// Created by quontas on 3/7/22.
//

#ifndef COG_GROUP_CONVO_CPP_CAPTIONS_HPP
#define COG_GROUP_CONVO_CPP_CAPTIONS_HPP

#include <string>
#include <mutex>
#include "cog-flatbuffer-definitions/caption_message_generated.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>

class CaptionModel {
private:
    std::vector<std::pair<cog::Juror, std::string>> spoken_so_far;
    std::mutex text_mutex;
    const static int LINE_LENGTH = 30;


    static std::string wrap(const std::string &text, const int line_length) {
        std::istringstream words(text);
        std::vector<std::string> wrapped_lines;
        std::string word;

        if (words >> word) {
            wrapped_lines.emplace_back(word);
            size_t space_left = line_length - word.length();
            while (words >> word) {
                if (space_left < word.length() + 1) {
                    wrapped_lines.emplace_back(word);
                    space_left = line_length - word.length();
                } else {
                    wrapped_lines.back() += ' ' + word;
                    space_left -= word.length() + 1;
                }
            }
        }
        if (wrapped_lines.empty()) {
            return "";
        }
        if (wrapped_lines.size() == 1) {
            return wrapped_lines.back();
        }
        return wrapped_lines.at(wrapped_lines.size() - 2) + '\n' + wrapped_lines.at(wrapped_lines.size() - 1);
    }

public:
    explicit CaptionModel() = default;

    void add_word(const std::string &new_word, cog::Juror speaker) {
        text_mutex.lock();
        if (!spoken_so_far.empty() && spoken_so_far.back().first != speaker) {
            spoken_so_far.clear();
        }
        spoken_so_far.emplace_back(speaker, new_word);
        text_mutex.unlock();
    }

    std::pair<cog::Juror, std::string> get_current_text(const int line_length = LINE_LENGTH) {
        std::string current_speech;
        cog::Juror current_juror = cog::Juror_JuryForeman;
        text_mutex.lock();
        if (!spoken_so_far.empty()) {
            current_juror = spoken_so_far.front().first;
            for (const auto&[_, word]: spoken_so_far) {
                current_speech += word + " ";
            }
        }
        text_mutex.unlock();
        return std::make_pair(current_juror, wrap(current_speech, line_length));
    }
};

cog::Juror juror_from_string(const std::string &juror_str) {
    cog::Juror juror;
    if (juror_str == "juror-a") {
        juror = cog::Juror_JurorA;
    } else if (juror_str == "juror-b") {
        juror = cog::Juror_JurorB;
    } else if (juror_str == "juror-c") {
        juror = cog::Juror_JurorC;
    } else if (juror_str == "jury-foreman") {
        juror = cog::Juror_JuryForeman;
    } else {
        std::cerr << "Unknown speaker ID encountered: " << juror_str << std::endl;
        throw;
    }
    return juror;
}

void play_captions(nlohmann::json *caption_json, CaptionModel *model) {
    for (auto i = 0; i < caption_json->size(); ++i) {
        auto text = caption_json->at(i)["text"].get<std::string>();
        double delay;
        if (i == 0) {
            delay = caption_json->at(i)["delay"].get<double>();
        } else {
            delay = caption_json->at(i)["delay"].get<double>() - caption_json->at(i - 1)["delay"].get<double>();
        }
        auto speaker_id_str = caption_json->at(i)["speaker_id"].get<std::string>();
        auto speaker_id = juror_from_string(speaker_id_str);
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1000>>(delay));
        model->add_word(text, speaker_id);
    }
}

#endif //COG_GROUP_CONVO_CPP_CAPTIONS_HPP
