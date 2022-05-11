#pragma once

#include "Color.h"

#include <stdint.h>

namespace Config
{
const uint8_t pin_servo = 9;
const uint8_t pin_led_r = 3;
const uint8_t pin_led_g = 5;
const uint8_t pin_led_b = 6;
const uint8_t led_ws = 7;
const uint8_t led_ws_count = 10;
const uint8_t min_servo_pos = 0;
const uint8_t max_servo_pos = 180;

const unsigned long m_refresh_time_ms = 30000;

const double latitude = 51.1078852;
const double longitude = 17.0385376;
const int8_t dst_offset = 2;

const Color horizon_sun(27, 4, 0);
const Color noon(255, 200, 0);
const Color blue_sky(0, 5, 12);
} // namespace Config