#include "math/gait_math.h"
#include "app_main.h"

float sind(float in) {
    return sinf(in * (M_PI / 180.0f));
}

float cosd(float in) {
    return cosf(in * (M_PI / 180.0f));
}

float To_180_degrees(float x) {
    return (x > 180.0f ? (x - 360.0f) : (x < -180.0f ? (x + 360.0f) : x));
}
