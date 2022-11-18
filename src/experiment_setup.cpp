#include "experiment_setup.hpp"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string>
#include <iostream>
#include <sstream>
#include <array>

/**
 * Prints a QR code to the console. The QR code's contents are formatted as follows:
 * "<MACHINE_IP_ADDR>:<PORT> <PRESENTATION_METHOD>"
 * @param presentation_method
 */
void print_connection_qr(int presentation_method, int port) {
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    getifaddrs(&ifap);
    // This is some witchcraft from StackOverflow
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            std::string interface = std::string(ifa->ifa_name);
            if (interface == "wlp3s0" || interface == "wlan0" || interface == "en0" || interface == "en1") {
                std::ostringstream command;
                std::string address = std::string(addr);
                std::string address_port = address + ":" + std::to_string(port);
                command << "qrencode -t ANSI \"" << addr << ":" << port << " " << presentation_method << "\"";
                std::cout << "Command is: " << command.str() << std::endl;
                system(command.str().c_str());
                break;
            } else {
                std::cout << "Interface: " << interface << " Address: " << addr << std::endl;
            }
        }
    }
    freeifaddrs(ifap);
}

std::tuple<int, sockaddr_in> connect_to_client(int port) {
    int sockfd;
    struct sockaddr_in servaddr{}, cliaddr{};

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    return std::make_tuple(sockfd, cliaddr);
}

SDL_Color color_string_to_color(const std::string &color_str) {
    SDL_Color result{0, 0, 0, 0};
    std::stringstream s_stream(color_str); //create string stream from the string
    std::string substr;
    getline(s_stream, substr, ','); //get first string delimited by comma
    result.r = std::stoi(substr);
    getline(s_stream, substr, ','); //get first string delimited by comma
    result.g = std::stoi(substr);
    getline(s_stream, substr, ','); //get first string delimited by comma
    result.b = std::stoi(substr);
    getline(s_stream, substr, ','); //get first string delimited by comma
    result.a = std::stoi(substr);
    return result;
}

std::tuple<int, int, int, SDL_Color, SDL_Color, std::string, int, int, int>
parse_arguments(int argc, char *argv[]) {
    int video_section;
    int presentation_method;
    int half_fov;
    SDL_Color foreground_color{0, 0, 0, 0};
    SDL_Color background_color{0, 0, 0, 0};
    std::string path_to_font;
    int fovPick;
    int num_chars;
    int font_size;
    int box_pixel_width;
    int cmd_opt;
    int option_index = 0;
    std::string fg_color_str;
    std::string bg_color_str;
    cmd_opt = getopt_long(argc, argv, "v:m:a:f:b:p:s:", long_options, &option_index);
    while (cmd_opt) {
        if (cmd_opt == -1) {
            break;
        }
        switch (cmd_opt) {
            case 'v':
                video_section = std::stoi(optarg);
                if (video_section <= 0 || video_section > 4) {
                    std::cerr << "Please pick a video section between 1-4." << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                presentation_method = std::stoi(optarg);
                break;
            case 'a':

                if (std::stoi(optarg) == 10 || std::stoi(optarg) == 20 || std::stoi(optarg) == 30 || std::stoi(optarg) == 40) {
                    half_fov = (int) std::stoi(optarg) / 2;
                } else {
                    std::cerr << "Please input FULL view angle of 10, 20, 30, or 40 degrees." << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'f':
                fg_color_str = std::string(optarg);
                foreground_color = color_string_to_color(fg_color_str);
                break;
            case 'b':
                bg_color_str = std::string(optarg);
                background_color = color_string_to_color(bg_color_str);
                break;
            case 'p':
                path_to_font = std::string(optarg);
                break;
            case '?':
            case 's':
                fovPick = (int) std::stoi(optarg);
                num_chars = fov_angle_to_num_chars(fovPick);
                box_pixel_width = fov_angle_to_box_pixel_width(fovPick);
                printf("FOV is %i\n", fovPick);
            default:
                std::cerr << "Unknown option received: " << cmd_opt << std::endl;
        }
        cmd_opt = getopt_long(argc, argv, "v:m:a:f:b:p:s:", long_options, &option_index);
    }
    return std::make_tuple(video_section, presentation_method, half_fov, foreground_color, background_color, path_to_font, fovPick, num_chars, box_pixel_width);
}

int fov_angle_to_num_chars(int angle) {
    // todo future work ... do the actual calculation here

    switch(angle) {
        case 15:
            return 29;
        case 20:
            return 39;
        case 25:
            return 49;
        case 30:
            return 60;
        default:
            std::cerr << "Choose FOV: 15, 20, 25, 30 " << std::endl;
    }
}

int fov_angle_to_box_pixel_width(int angle) {
    switch(angle) {
        case 15:
            return 502;
        case 20:
            return 673;
        case 25:
            return 846;
        case 30:
            return 1023;
        default:
            std::cerr << "Choose FOV: 15, 20, 25, 30 " << std::endl;
    }
}

