#include <cmath>
#include <deque>
#include <iostream>
#include "orientation.hpp"
#include "cog-flatbuffer-definitions/orientation_message_generated.h"

int to_pixels(double inches) {
    return inches * PIXELS_PER_INCH;
}

int angle_to_pixel_position(double angle) {
    const auto position_in_inches = std::tan(angle + PI/2) * INCHES_FROM_SCREEN;
    return to_pixels(position_in_inches);
}


void read_orientation(int socket, sockaddr_in *client_address, std::mutex *socket_mutex, std::mutex *azimuth_mutex,
                      std::deque<float> *orientation_buffer) {
    size_t len, num_bytes_read;
    std::array<char, 1024> buffer{};
    len = sizeof(*client_address);

    socket_mutex->lock();
    num_bytes_read = recvfrom(socket, buffer.data(), buffer.size(),
                              MSG_WAITALL, (struct sockaddr *) &(*client_address),
                              reinterpret_cast<socklen_t *>(&len));
    socket_mutex->unlock();
    while (num_bytes_read != -1) {
        azimuth_mutex->lock();
        if (orientation_buffer->size() == MOVING_AVG_SIZE) {
            orientation_buffer->pop_front();
        }
        auto current_orientation = cog::GetOrientationMessage(buffer.data());
        orientation_buffer->push_back(current_orientation->azimuth());
        azimuth_mutex->unlock();
        socket_mutex->lock();
        if (recvfrom(socket, buffer.data(), buffer.size(),
                     MSG_WAITALL, (struct sockaddr *) &(*client_address),
                     reinterpret_cast<socklen_t *>(&len)) < 0) {
            std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
        }
        socket_mutex->unlock();
    }
}