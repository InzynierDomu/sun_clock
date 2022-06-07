/**
 * @file Config.h
 * @brief Configuration
 * @author by Szymon Markiewicz
 * @details http://www.inzynierdomu.pl/
 * @date 05-2022
 */

#pragma once

#include "Color.h"

#include <stdint.h>

namespace Config
{
const uint8_t pin_servo = 9; ///< pin to controll pwm for servo
const uint8_t pin_led_r = 3; ///< red pin in RGB LED
const uint8_t pin_led_g = 5; ///< green pin in RGB LED
const uint8_t pin_led_b = 6; ///< blue pin in RGB LED
const uint8_t led_ws = 7; ///< pin for WS2812 LED
const uint8_t led_ws_count = 10; ///< WS2812 LED count
const uint8_t min_servo_pos = 0; ///< minimal servo position
const uint8_t max_servo_pos = 180; ///< maximum servo position

const unsigned long m_refresh_time_ms = 30000; ///< refrishing time in main loop

const double latitude = 51.1078852; ///< latitude loaction
const double longitude = 17.0385376; ///< longitude loaction
const int8_t dst_offset = 2; ///< daylight saving time offset

const Color horizon_sun(27, 4, 0); ///< sun color when it's on horizon
const Color noon(255, 200, 0); ///< sun color when is noon
const Color blue_sky(0, 5, 12); ///< sky color on day
} // namespace Config