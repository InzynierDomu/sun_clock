/**
 * @file main.cpp
 * @brief clock tracking the position and color of the sun and sky
 * @author by Szymon Markiewicz
 * @details http://www.inzynierdomu.pl/
 * @date 05-2022
 */

#include "Adafruit_NeoPixel.h"
#include "Color.h"
#include "Config.h"
#include "RTClib.h"
#include "sunset.h"

#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <math.h>
#include <stdint.h>

///< day part
enum class Day_part
{
  night,
  sunrise,
  before_noon,
  after_noon,
  sunset
};

struct Point
{
  Point()
  : time(0)
  {}
  Point(uint16_t _time, Color _color)
  : time(_time)
  , color(_color)
  {}
  uint16_t time; ///< time in minutes from 0:00
  Color color; ///< color
};

struct Sun_position
{
  Point sunrise_civil;
  Point sunrise;
  Point noon;
  Point sunset;
  Point sunset_civil;
};

const uint8_t m_min_in_h = 60; ///< minutes in hour

Sun_position sun_position; ///< characteristic points for the sun on sky

RTC_DS1307 m_rtc; ///< DS1307 RTC
SunSet sun; ///< Sun position calculation
Servo m_servo; ///< HW servo
Adafruit_NeoPixel m_ws_leds(Config::led_ws_count, Config::led_ws, NEO_GRB + NEO_KHZ800); ///< WS2812 leds (sky)

/**
 * @brief calculate minutes form 0:00 to hour and minutes
 * @param total_min: minutes form 0:00
 * @return DateTime calculated time
 */
DateTime calculate_from_minutes(uint16_t total_min)
{
  auto minutes = total_min % m_min_in_h;
  auto houres = (total_min - minutes) / m_min_in_h;
  return DateTime(1970, 1, 1, houres, minutes);
}

/**
 * @brief print time on Serial hh:mm:ss
 * @param time: time to print
 */
void print_time(DateTime time)
{
  Serial.print(time.hour(), DEC);
  Serial.print(":");
  Serial.print(time.minute(), DEC);
  Serial.print(":");
  Serial.println(time.second(), DEC);
}

/**
 * @brief calculate sunrise and sunset times
 */
void calculate_sunrise_sunset()
{
  uint16_t sunrise = static_cast<uint16_t>(sun.calcSunrise());
  uint16_t sunset = static_cast<uint16_t>(sun.calcSunset());
  uint16_t sunrise_civil = static_cast<uint16_t>(sun.calcCivilSunrise());
  uint16_t sunset_civil = static_cast<uint16_t>(sun.calcCivilSunset());
  Serial.print("Sunrise civil: ");
  print_time(calculate_from_minutes(sunrise_civil));
  Serial.print("Sunrise: ");
  print_time(calculate_from_minutes(sunrise));
  Serial.print("Sunset: ");
  print_time(calculate_from_minutes(sunset));
  Serial.print("Sunset civil: ");
  print_time(calculate_from_minutes(sunset_civil));

  Serial.print("middle: ");
  uint16_t day_middle = ((sunset - sunrise) / 2) + sunrise;
  print_time(calculate_from_minutes(day_middle));

  sun_position.sunrise_civil = Point(sunrise_civil, Color());
  sun_position.sunrise = Point(sunrise, Config::horizon_sun);
  sun_position.noon = Point(day_middle, Config::noon);
  sun_position.sunset = Point(sunset, Config::horizon_sun);
  sun_position.sunset_civil = Point(sunset_civil, Color());
}

/**
 * @brief calculate minutes form 0:00
 * @param time: time to calculate
 * @return uint16_t calculated minutes
 */
uint16_t calculate_from_datetime(DateTime time)
{
  return (time.hour() * m_min_in_h) + time.minute();
}

/**
 * @brief calculate servo angle depending to time
 * @param now: time in minutes from 0:00
 * @param actual_day_part: what part of day (sun on sky) is now
 */
uint8_t calculate_servo_position(uint16_t now, const Day_part actual_day_part)
{
  if (actual_day_part == Day_part::sunset)
  {
    return Config::max_servo_pos;
  }
  else if (actual_day_part == Day_part::sunrise || actual_day_part == Day_part::night)
  {
    return Config::min_servo_pos;
  }
  else
  {
    return map(now, sun_position.sunrise.time, sun_position.sunset.time, Config::min_servo_pos, Config::max_servo_pos);
  }
}

/**
 * @brief move servo to angle depending to time
 * @param now: time in minutes from 0:00
 * @param actual_day_part: what part of day (sun on sky) is now
 */
void move_servo(uint16_t now, const Day_part actual_day_part)
{
  m_servo.attach(Config::pin_servo);
  uint8_t servo_position = calculate_servo_position(now, actual_day_part);

  Serial.print("servo pos: ");
  Serial.println(servo_position);

  m_servo.write(servo_position);
  delay(Config::time_for_servo_move);
  m_servo.detach();
}

/**
 * @brief calculate regarding sin sunftion, color more linear for eye
 * @param x: value for calculation
 * @param max: maximum value for the output
 * @return uint16_t return value from calculation
 */
uint16_t sin_fun(long x, uint8_t max)
{
  return double(max) * (sin((double(x - max) * M_PI) / double(max * 2))) + double(max);
}

/**
 * @brief fit parameters to mathematical function
 * @param now: time in minutes from 0:00
 * @param min_time: minimal input time(now)
 * @param max_time: maximum input time(now)
 * @param min_color: minimal calor staurations to output
 * @param max_color: maximum calor staurations to output
 * @param is_rising: function direction
 * @return Color output from mathematical function
 */
Color map_on_function(uint16_t now, Point min_time, Point max_time, Color min_color, Color max_color, bool is_rising)
{
  Color retval;
  Color max_fun_color;

  if (is_rising)
  {
    max_fun_color = max_color;
  }
  else
  {
    max_fun_color = min_color;
  }

  auto y = map(now, min_time.time, max_time.time, min_color.r, max_color.r);
  retval.r = sin_fun(y, max_fun_color.r);
  y = map(now, min_time.time, max_time.time, min_color.g, max_color.g);
  retval.g = sin_fun(y, max_fun_color.g);
  y = map(now, min_time.time, max_time.time, min_color.b, max_color.b);
  retval.b = sin_fun(y, max_fun_color.b);

  return retval;
}

/**
 * @brief Get the sky horizon rgb
 * @param now: time in minutes from 0:00
 * @param is_rising: true = before sunrise; false = after sunset
 * @return Color sky color
 */
Color get_sky_horizon_rgb(uint16_t now, bool is_rising)
{
  Color night;
  if (is_rising)
  {
    return map_on_function(now, sun_position.sunrise_civil, sun_position.sunrise, night, Config::blue_sky, is_rising);
  }
  return map_on_function(now, sun_position.sunset, sun_position.sunset_civil, Config::blue_sky, night, is_rising);
}

/**
 * @brief Set the sky rgb
 * @param now: time in minutes from 0:00
 * @param actual_day_part: what part of day (sun on sky) is now
 */
void set_sky_rgb(uint16_t now, const Day_part actual_day_part)
{
  Color color;

  switch (actual_day_part)
  {
    case Day_part::sunset:
      color = get_sky_horizon_rgb(now, false);
      break;
    case Day_part::sunrise:
      color = get_sky_horizon_rgb(now, true);
      break;
    case Day_part::before_noon:
      [[fallthrough]];
    case Day_part::after_noon:
      color = Config::blue_sky;
      break;
    default:
      break;
  }

  m_ws_leds.fill(color.get_color());
  m_ws_leds.show();
}

/**
 * @brief Get the sun horizon rgb
 * @param now: time in minutes from 0:00
 * @param is_rising: true = before sunrise; false = after sunset
 * @return Color sun color on specify position
 */
Color get_sun_horizon_rgb(uint16_t now, bool is_rising)
{
  // Color color;
  if (is_rising)
  {
    return map_on_function(
        now, sun_position.sunrise_civil, sun_position.sunrise, sun_position.sunrise_civil.color, sun_position.sunrise.color, is_rising);
  }
  return map_on_function(
      now, sun_position.sunset, sun_position.sunset_civil, sun_position.sunset.color, sun_position.sunset_civil.color, is_rising);
}

/**
 * @brief Get the sun day rgb
 * @param now: time in minutes from 0:00
 * @param is_afternoon: true = afternoon; false = before noon
 * @return Color sun color on specify position
 */
Color get_sun_day_rgb(uint16_t now, bool is_afternoon)
{
  Color color;
  if (is_afternoon)
  {
    color.r = map(now, sun_position.sunrise.time, sun_position.noon.time, sun_position.sunrise.color.r, sun_position.noon.color.r);
    color.g = map(now, sun_position.sunrise.time, sun_position.noon.time, sun_position.sunrise.color.g, sun_position.noon.color.g);
    return color;
  }
  color.r = map(now, sun_position.noon.time, sun_position.sunset.time, sun_position.noon.color.r, sun_position.sunset.color.r);
  color.g = map(now, sun_position.noon.time, sun_position.sunset.time, sun_position.noon.color.g, sun_position.sunset.color.g);
  return color;
}

/**
 * @brief check current day part
 * @param now: time in minutes from 0:00
 */
Day_part check_day_part(uint16_t now)
{
  if (now > sun_position.sunset_civil.time)
  {
    return Day_part::night;
  }
  else if (now > sun_position.sunset.time)
  {
    return Day_part::sunset;
  }
  else if (now > sun_position.noon.time)
  {
    return Day_part::after_noon;
  }
  else if (now > sun_position.sunrise.time)
  {
    return Day_part::before_noon;
  }
  else if (now > sun_position.sunrise_civil.time)
  {
    return Day_part::sunrise;
  }
  else
  {
    return Day_part::night;
  }
}

/**
 * @brief Set the sun rgb object
 * @param now: time in minutes from 0:00
 * @param actual_day_part: what part of day (sun on sky) is now
 */
void set_sun_rgb(uint16_t now, const Day_part actual_day_part)
{
  Color color;

  switch (actual_day_part)
  {
    case Day_part::sunset:
      color = get_sun_horizon_rgb(now, false);
      break;
    case Day_part::after_noon:
      color = get_sun_day_rgb(now, false);
      break;
    case Day_part::before_noon:
      color = get_sun_day_rgb(now, true);
      break;
    case Day_part::sunrise:
      color = get_sun_horizon_rgb(now, true);
      break;
    default:
      break;
  }

  analogWrite(Config::pin_led_r, color.r);
  analogWrite(Config::pin_led_g, color.g);
  analogWrite(Config::pin_led_b, color.b);
}

/**
 * @brief setup
 */
void setup()
{
  Serial.begin(Config::serial_baudrate);

  if (!m_rtc.begin())
  {
    Serial.println("Couldn't find RTC");
  }

  m_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  sun.setPosition(Config::latitude, Config::longitude, Config::dst_offset);
  auto now = m_rtc.now();
  sun.setCurrentDate(now.year(), now.month(), now.day());

  pinMode(Config::pin_led_r, OUTPUT);
  pinMode(Config::pin_led_g, OUTPUT);
  pinMode(Config::pin_led_b, OUTPUT);

  m_ws_leds.begin();
}

/**
 * @brief main loop
 */
void loop()
{
  static unsigned long last_loop_time = 0;
  unsigned long loop_time = millis();
  if (loop_time - last_loop_time > Config::m_refresh_time_ms)
  {
    auto now = m_rtc.now();

    Serial.print("Now: ");
    print_time(now);
    last_loop_time = millis();

    auto minutes = calculate_from_datetime(now);

    static bool is_calculated = false;
    if (minutes == 1 || !is_calculated)
    {
      calculate_sunrise_sunset();
      is_calculated = true;
    }

    Day_part actual_day_part = check_day_part(minutes);

    set_sun_rgb(minutes, actual_day_part);

    move_servo(minutes, actual_day_part);

    set_sky_rgb(minutes, actual_day_part);
  }
}