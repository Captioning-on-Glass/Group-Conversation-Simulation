#include <cmath>
#include <deque>
#include <iostream>
#include "orientation.hpp"
#include "cog-flatbuffer-definitions/orientation_message_generated.h"

bool bounds_recorded = false;
const double PIXEL_JIGGLE_THRESHOLD = 500;
double theta_left, theta_right, theta_top, theta_bottom;
std::deque<float> pitch_buffer;


int to_pixels(double inches) {
    return inches * PIXELS_PER_INCH;
}

int angle_to_pixel_position(double angle) {
    const auto position_in_inches = std::tan(angle) * INCHES_FROM_SCREEN;
    return to_pixels(position_in_inches);
}

double to_radians(double degrees) {
    return degrees * PI / 180.f;
}

double get_average_of(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex)
{
    double sum = std::accumulate(azimuth_buffer->begin(), azimuth_buffer->end(), 0.0);
    double n = azimuth_buffer->size();
    return sum / n;
}

double pixel_mapped(double angle, const AppContext *context)
{
    //previous pixel, check if current pixel is greater then previous pixel + moving_limiter
    //if true, then we return new pixel, else we return old pixel
    // TODO: stop "jiggle" by not changing the pixel, unless the angle has changed a certain ammount

    if (!bounds_recorded)
    {
        theta_left = context->left_bound;
        theta_right = context->right_bound;
        theta_top = context->top_bound;
        theta_bottom = context->bottom_bound;
        bounds_recorded = true;
    }

    int pixel = ( 3840 / ( theta_right - theta_left ) ) * ( angle - theta_left );
    int avg_pixel = ( 3840 / ( theta_right - theta_left ) ) * (get_average_of(context->azimuth_buffer, context->azimuth_mutex) - theta_left );

    if (abs(pixel - avg_pixel) > PIXEL_JIGGLE_THRESHOLD)
    {
        return pixel;
    }
    else
    {
        return avg_pixel;
    }

        return pixel;
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
        if (orientation_buffer->size() == MOVING_AVG_SIZE) orientation_buffer->pop_front();
        if (pitch_buffer.size() == MOVING_AVG_SIZE) pitch_buffer.pop_front();

        auto current_orientation = cog::GetOrientationMessage(buffer.data());
        auto current_azimuth = current_orientation->azimuth();
        auto current_pitch = current_orientation->pitch();

        if (current_azimuth < 0) current_azimuth = current_azimuth + 2 * PI;
        current_pitch = current_pitch + PI / 2;
        orientation_buffer->push_back(current_azimuth);
        pitch_buffer.push_back(current_pitch);
        azimuth_mutex->unlock();
        if (recvfrom(socket, buffer.data(), buffer.size(),
                     0, (struct sockaddr *) &(*client_address),
                     reinterpret_cast<socklen_t *>(&len)) < 0) {
            std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
        }
    }
}

double filtered_pitch(std::mutex *azimuth_mutex){
    if (pitch_buffer.empty()) return 0;
    azimuth_mutex->lock();
    double angle = get_average_of(&pitch_buffer, azimuth_mutex);
    azimuth_mutex->unlock();
    return angle;
}
double filtered_azimuth(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex) {
    azimuth_mutex->lock();
    if (azimuth_buffer->empty()) {
        azimuth_mutex->unlock();
        return 0;
    }
    double average_azimuth = get_average_of(azimuth_buffer, azimuth_mutex);
    auto angle = average_azimuth;
    azimuth_mutex->unlock();
    return angle;
}
