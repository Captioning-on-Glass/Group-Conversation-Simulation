#include <string>
#include <iostream>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sstream>
#include <tuple>
#include <cstring>
#include <getopt.h>
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
            if (interface == "wlp3s0" || interface == "wlan0" || interface == "en0") {
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
        perror("socket creation failed");
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

using RGBA = std::array<uint8_t, 4>;

RGBA color_string_to_rgba(const std::string &color_str) {
    RGBA result{0, 0, 0, 0};
    std::stringstream s_stream(color_str); //create string stream from the string
    for (auto &elem: result) {
        std::string substr;
        getline(s_stream, substr, ','); //get first string delimited by comma
        elem = std::stoi(substr);
    }
    return result;
}

static struct option long_options[] = {
        {"video_section",       required_argument, nullptr, 'v'},
        {"presentation_method", required_argument, nullptr, 'm'},
        {"foreground_color",    required_argument, nullptr, 'f'},
        {"background_color",    required_argument, nullptr, 'b'},
        {"path_to_font",        required_argument, nullptr, 'p'},
        {"font_size",           required_argument, nullptr, 's'}
};

std::tuple<int, int, RGBA, RGBA, std::string, int>
parse_arguments(int argc, char *argv[]) {
    int video_section;
    int presentation_method;
    RGBA foreground_color{0, 0, 0, 0};
    RGBA background_color{0, 0, 0, 0};
    std::string path_to_font;
    int font_size;
    int cmd_opt;
    int option_index = 0;
    std::string fg_color_str;
    std::string bg_color_str;
    cmd_opt = getopt_long(argc, argv, "v:m:f:b:p:s:", long_options, &option_index);
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
            case 'f':
                fg_color_str = std::string(optarg);
                foreground_color = color_string_to_rgba(fg_color_str);
                break;
            case 'b':
                bg_color_str = std::string(optarg);
                background_color = color_string_to_rgba(bg_color_str);
                break;
            case 'p':
                path_to_font = std::string(optarg);
                break;
            case 's':
                font_size = std::stoi(optarg);
                break;
            case '?':
            default:
                std::cerr << "Unknown option received: " << cmd_opt << std::endl;
        }
        cmd_opt = getopt_long(argc, argv, "v:m:f:b:p:s:", long_options, &option_index);
    }
    return std::make_tuple(video_section, presentation_method, foreground_color, background_color, path_to_font,
                           font_size);
}