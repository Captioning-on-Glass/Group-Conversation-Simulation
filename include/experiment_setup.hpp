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


static struct option long_options[] = {
        {"video_section", required_argument, nullptr, 'v'},
        {"blur_level",    required_argument, nullptr, 'l'},
};

std::tuple<int, int>
parse_arguments(int argc, char *argv[]);

#endif //COG_GROUP_CONVO_CPP_EXPERIMENT_SETUP_HPP
