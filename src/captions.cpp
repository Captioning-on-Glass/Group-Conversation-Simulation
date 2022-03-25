#include <thread>
#include <iostream>
#include <filesystem>
#include <SDL_image.h>
#include "captions.hpp"

CaptionModel::CaptionModel(SDL_Renderer *renderer, const std::string &path_to_bitmaps,
                           const std::vector<cog::Juror> &speaker_ids, const int blur_level) {
    // Written on a plane w/o wifi, this can be done more cleanly.

    for (auto i = 0; i < speaker_ids.size(); ++i) {
        const std::string filepath =
                path_to_bitmaps + "/" + std::to_string(i) + "." + std::to_string(blur_level) + ".png";
        if (!std::filesystem::exists(filepath)) {
            break;
        }
        this->captions.emplace_back(IMG_Load(filepath.c_str()), speaker_ids.at(i));
    }
}

CaptionModel::~CaptionModel() {
    for (auto [surface, juror] : this->captions) {
        SDL_FreeSurface(surface);
    }
}

void CaptionModel::increment() {
    this->idx++;
}

std::pair<SDL_Surface*, cog::Juror> CaptionModel::get_current_caption() {
    return this->captions.at(this->idx);
}

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

void transmit_caption(int socket, sockaddr_in *client_address, std::mutex *socket_mutex, const std::string &text,
                      cog::Juror speaker_id, cog::Juror focused_id, int message_id, int chunk_id) {
    flatbuffers::FlatBufferBuilder builder(1024);
    auto caption_message = cog::CreateCaptionMessageDirect(builder, text.c_str(), speaker_id, focused_id, message_id,
                                                           chunk_id);
    builder.Finish(caption_message);
    uint8_t *buffer = builder.GetBufferPointer();
    const auto size = builder.GetSize();
    socklen_t len = sizeof(*client_address);

    socket_mutex->lock();
    if (sendto(socket, buffer, 1024, 0, (struct sockaddr *) &(*client_address),
               len) < 0) {
        std::cerr << "sendto failed: " << strerror(errno) << std::endl;
    }
    socket_mutex->unlock();
}


void
start_caption_stream(const bool *started, int socket, sockaddr_in *client_address, std::mutex *socket_mutex,
                     nlohmann::json *caption_json, CaptionModel *model) {
    while (!(*started)) {}
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
        auto message_id = caption_json->at(i)["message_id"].get<int>();
        auto chunk_id = caption_json->at(i)["chunk_id"].get<int>();
        auto focused_id = cog::Juror_JuryForeman;
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1000>>(delay));
//        transmit_caption(socket, client_address, socket_mutex, text, speaker_id, focused_id, message_id, chunk_id);
        model->increment();
    }
}
