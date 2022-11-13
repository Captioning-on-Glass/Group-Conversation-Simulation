#include <cmath>
#include <deque>
#include <iostream>
#include "orientation.hpp"
#include "cog-flatbuffer-definitions/orientation_message_generated.h"

double current_caption_pixel = 0;
bool is_moving = false;
const double PIXEL_JIGGLE_THRESHOLD = 75;
double prev_filtered_angle = 0;
double ALPHA = 0.1;

int to_pixels(double inches) {
    return inches * PIXELS_PER_INCH;
}

int angle_to_pixel_position(double angle) {
    const auto position_in_inches = std::tan(angle) * INCHES_FROM_SCREEN;
    return to_pixels(position_in_inches);
}

double get_average_of(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex)
{
    double sum = std::accumulate(azimuth_buffer->begin(), azimuth_buffer->end(), 0.0);
    double n = azimuth_buffer->size();
    return sum / n;
}

double to_radians(double degrees) {
    return degrees * PI / 180.f;
}

double pixel_mapped(double angle, const AppContext *context)
{
    //previous pixel, check if current pixel is greater then previous pixel + moving_limiter
    //if true, then we return new pixel, else we return old pixel
    // TODO: stop "jiggle" by not changing the pixel, unless the angle has changed a certain ammount


    auto theta_left = context->left_bound;
    auto theta_right = context->right_bound;

    int pixel = ( 3840 / ( theta_right - theta_left ) ) * ( angle - theta_left );

//    printf("angle: %f\n", angle);
//    printf("current caption pixel: %f\n", current_caption_pixel);
//    printf("pixel: %f\n", pixel);
//    filtered_azimuth(context->azimuth_buffer, context->azimuth_mutex)


//    if (current_caption_pixel + PIXEL_JIGGLE_THRESHOLD < pixel or pixel < current_caption_pixel - PIXEL_JIGGLE_THRESHOLD) {
//        current_caption_pixel = pixel;
        //is_moving = true;
        return pixel;
//    } else {
//        //if(is_moving){
//        //    current_caption_pixel = pixel;
//        //    is_moving = true;
//        //    return pixel;
//        //}
//        //is_moving = false;
////        return current_caption_pixel;
//        return current_caption_pixel;

//    }

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
        if (current_azimuth < 0) {
            current_azimuth = current_azimuth + 2 * PI;
        }
        orientation_buffer->push_back(current_azimuth);
        azimuth_mutex->unlock();
        if (recvfrom(socket, buffer.data(), buffer.size(),
                     0, (struct sockaddr *) &(*client_address),
                     reinterpret_cast<socklen_t *>(&len)) < 0) {
            std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
        }
    }
}

float current_orientation(int socket, sockaddr_in *client_address, std::mutex *azimuth_mutex,
                      std::deque<float> *orientation_buffer) {
    size_t len, num_bytes_read;
    std::array<char, 256> buffer{};
    len = sizeof(*client_address);

    num_bytes_read = recvfrom(socket, buffer.data(), buffer.size(),
                              0, (struct sockaddr *) &(*client_address),
                              reinterpret_cast<socklen_t *>(&len));
    float current_azimuth;

    while (num_bytes_read != -1) {
        azimuth_mutex->lock();
        if (orientation_buffer->size() == MOVING_AVG_SIZE) {
            orientation_buffer->pop_front();
        }
        auto current_orientation = cog::GetOrientationMessage(buffer.data());
        current_azimuth = current_orientation->azimuth();
        if (current_azimuth < 0) {
            current_azimuth = current_azimuth + 2 * PI;
        }
        orientation_buffer->push_back(current_azimuth);
        azimuth_mutex->unlock();
        if (recvfrom(socket, buffer.data(), buffer.size(),
                     0, (struct sockaddr *) &(*client_address),
                     reinterpret_cast<socklen_t *>(&len)) < 0) {
            std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
        }
    }

    return current_azimuth;
}

double filtered_azimuth(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex)
{
    bool buffer_has_content = !azimuth_buffer->empty();
    double filtered_angle = 0.0;

    if (buffer_has_content)
    {
        azimuth_mutex->lock();
        filtered_angle = get_average_of(azimuth_buffer, azimuth_mutex);
        azimuth_mutex->unlock();
    }

    return filtered_angle;
}


bool far_enough(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex)
{
    bool significant_movement = false;
    bool buffer_has_content = !azimuth_buffer->empty();
    double average_azimuth;
    double delta_azimuth;

    if (buffer_has_content)
    {
        azimuth_mutex->lock();
        average_azimuth = get_average_of(azimuth_buffer, azimuth_mutex);
        delta_azimuth = average_azimuth - azimuth_buffer->back();
        significant_movement = delta_azimuth > PIXEL_JIGGLE_THRESHOLD;
        std::cout<< delta_azimuth;
        azimuth_mutex->unlock();
    }
    return significant_movement;
}

double exponential_filtered_azimuth(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex) // todo current_orientatoin?
{
    bool significant_movement_detected = far_enough(azimuth_buffer, azimuth_mutex);
    double filtered_current_angle = 0.0;

    if (significant_movement_detected)
    {
        azimuth_mutex->lock();
        double current_angle = azimuth_buffer->back();// todo is this supposed to be front or back?
        filtered_current_angle = (current_angle * ALPHA ) + (prev_filtered_angle * (1 - ALPHA ) );//todo put back to prev_filtered_angle
        prev_filtered_angle = filtered_current_angle;
        azimuth_mutex->unlock();
    }
    return filtered_current_angle;
}