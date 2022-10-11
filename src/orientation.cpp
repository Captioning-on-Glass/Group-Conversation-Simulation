#include <cmath>
#include <deque>
#include <iostream>
#include "orientation.hpp"
#include "cog-flatbuffer-definitions/orientation_message_generated.h"

int to_pixels(double inches) {
    return inches * PIXELS_PER_INCH;
}

int angle_to_pixel_position(double angle) {
    const auto position_in_inches = std::tan(angle) * INCHES_FROM_SCREEN;
//    return to_pixels(position_in_inches);
    return 3840 * tan(angle + PI/2);
}

double to_radians(double degrees) {
    return degrees * PI / 180.f;
}


void read_orientation(int socket, sockaddr_in *client_address, std::mutex *azimuth_mutex,
                      std::deque<float> *orientation_buffer) {
    size_t len, num_bytes_read;
    std::array<char, 256> buffer{};
    len = sizeof(*client_address);

    num_bytes_read = recvfrom(socket, buffer.data(), buffer.size(),
                              0, (struct sockaddr *) &(*client_address),
                              reinterpret_cast<socklen_t *>(&len));
    while (num_bytes_read != -1) {
        azimuth_mutex->lock();
        if (orientation_buffer->size() == MOVING_AVG_SIZE) {
            orientation_buffer->pop_front();
        }
        auto current_orientation = cog::GetOrientationMessage(buffer.data());
        auto current_azimuth = current_orientation->azimuth();

        std::cout<<"Azimuth is: " << current_azimuth << "\n";
//        if (current_azimuth < 0) {
//            current_azimuth = current_azimuth + 2 * PI;
//        }
        orientation_buffer->push_back(current_azimuth);
        azimuth_mutex->unlock();
        if (recvfrom(socket, buffer.data(), buffer.size(),
                     0, (struct sockaddr *) &(*client_address),
                     reinterpret_cast<socklen_t *>(&len)) < 0) {
            std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
        }
    }
}

double filtered_azimuth(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex) {
    azimuth_mutex->lock();
    if (azimuth_buffer->empty()) {
        azimuth_mutex->unlock();
        return 0;
    }
    double average_azimuth =
            std::accumulate(azimuth_buffer->begin(), azimuth_buffer->end(), 0.0) /
            azimuth_buffer->size();
//    double median_azimuth = median(azimuth_buffer);
    auto angle = average_azimuth;
    azimuth_mutex->unlock();
    return angle;
}