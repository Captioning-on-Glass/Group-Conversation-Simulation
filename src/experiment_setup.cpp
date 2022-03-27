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

std::tuple<int, int, int, int>
parse_arguments(int argc, char *argv[]) {
    int video_section;
    int presentation_method;
    int blur_level = 0;
    int opacity = 0;
    int cmd_opt;
    int option_index = 0;
    cmd_opt = getopt_long(argc, argv, "v:m:l:o:", long_options, &option_index);
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
            case 'l':
                blur_level = std::stoi(optarg);
                break;
            case 'o':
                opacity = std::stoi(optarg);
                break;
            case '?':
            default:
                std::cerr << "Unknown option received: " << (char)cmd_opt << std::endl;
        }
        cmd_opt = getopt_long(argc, argv, "v:m:l:o:", long_options, &option_index);
    }
    return std::make_tuple(video_section, presentation_method, blur_level, opacity);
}