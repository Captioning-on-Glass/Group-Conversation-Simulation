#ifndef COG_GROUP_CONVO_CPP_ORIENTATION_HPP
#define COG_GROUP_CONVO_CPP_ORIENTATION_HPP

#include <deque>
#include <netinet/in.h>
#include <mutex>
#include "AppContext.hpp"

const static int INCHES_FROM_SCREEN = 24; // inches
constexpr int SCREEN_PIXEL_WIDTH = 3840;
constexpr int SCREEN_PIXEL_HEIGHT = 2160;
constexpr double PIXELS_PER_INCH = 100.f /
                                   1.6f; // 100 pixels / 1.6 in (calculated empirically by measuring the width (in inches) of a 100px rectangle, see "ppi" branch)
constexpr size_t MOVING_AVG_SIZE = 500;

constexpr double PI = 3.14159265358979323846;

int to_pixels(double inches);

int angle_to_pixel_position(double angle);

double pixel_mapped(double angle, const AppContext *context);
double pixel_mapped_y(double angle, const AppContext *context);
double to_radians(double degrees);

void read_orientation(int socket, sockaddr_in *client_address, std::mutex *azimuth_mutex,
                      std::deque<float> *orientation_buffer);

double filtered_pitch(std::mutex *azimuth_mutex);

double filtered_azimuth(std::deque<float> *azimuth_buffer, std::mutex *azimuth_mutex);


#endif //COG_GROUP_CONVO_CPP_ORIENTATION_HPP
