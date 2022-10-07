//
// Created by Gabriel Britain on 3/16/22.
//

#ifndef COG_GROUP_CONVO_CPP_EXPERIMENT_SETUP_HPP
#define COG_GROUP_CONVO_CPP_EXPERIMENT_SETUP_HPP

#include <tuple>
#include <netinet/in.h>
#include <getopt.h>
#include <SDL.h>

/**
 * Prints a QR code to the console. The QR code's contents are formatted as follows:
 * "<MACHINE_IP_ADDR>:<PORT> <PRESENTATION_METHOD>"
 * @param presentation_method
 */
void print_connection_qr(int presentation_method, int port);

std::tuple<int, sockaddr_in> connect_to_client(int port);


SDL_Color color_string_to_color(const std::string &color_str);

static struct option long_options[] = {
        {"video_section",       required_argument, nullptr, 'v'},
        {"presentation_method", required_argument, nullptr, 'm'},
        {"angle_fov",            required_argument, nullptr, 'a'},
        {"foreground_color",    required_argument, nullptr, 'f'},
        {"background_color",    required_argument, nullptr, 'b'},
        {"path_to_font",        required_argument, nullptr, 'p'},
};

std::tuple<int, int, int, SDL_Color, SDL_Color, std::string>
parse_arguments(int argc, char *argv[]);

std::tuple<int, int, int> calibrate_head_movement();

#endif //COG_GROUP_CONVO_CPP_EXPERIMENT_SETUP_HPP
