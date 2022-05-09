#include "Adafruit_NeoPixel.h"
#include "Config.h"
#include "RTClib.h"
#include "sunset.h"

#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <math.h>
#include <stdint.h>

enum class Colors
{
  red,
  green,
  blue
};

enum class Day_part
{
  night,
  sunrise,
  before_noon,
  after_noon,
  sunset
};

struct Color
{
  Color(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0)
  : r(_r)
  , g(_g)
  , b(_b)
  {}
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint32_t get_color() const
  {
    uint32_t color = b;
    color += g << 8;
    color += r << 16;
    return color;
  }
};

struct Point
{
  Point() {}
  Point(uint16_t _time, Color _color)
  : time(_time)
  , color(_color)
  {}
  uint16_t time;
  Color color;
};

struct Sun_position
{
  Point night;
  Point sunrise_civil;
  Point sunrise;
  Point noon;
  Point sunset;
  Point sunset_civil;
};

const unsigned long m_refresh_time_ms = 15000;

const uint8_t m_min_in_h = 60;

Sun_position sun_position;

Day_part actual_day_part;

RTC_DS1307 m_rtc; ///< DS1307 RTC
SunSet sun;
Servo m_servo;
Adafruit_NeoPixel m_ws_leds(Config::led_ws_count, Config::led_ws, NEO_GRB + NEO_KHZ800);

DateTime calculate_from_minutes(uint16_t total_min)
{
  m_servo.attach(Config::pin_servo);

  auto minutes = total_min % m_min_in_h;
  auto houres = (total_min - minutes) / m_min_in_h;
  return DateTime(1970, 1, 1, houres, minutes);
}

void print_time(DateTime time)
{
  Serial.print(time.hour(), DEC);
  Serial.print(":");
  Serial.print(time.minute(), DEC);
  Serial.print(":");
  Serial.println(time.second(), DEC);
}

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

  sun_position.night = Point(0, Color());
  sun_position.sunrise_civil = Point(sunrise_civil, Color());
  sun_position.sunrise = Point(sunrise, Color(27, 2, 0));
  sun_position.noon = Point(day_middle, Color(255, 220, 0));
  sun_position.sunset = Point(sunset, Color(27, 2, 0));
  sun_position.sunset_civil = Point(sunset_civil, Color());
}

uint16_t calculate_from_datetime(DateTime time)
{
  return (time.hour() * 60) + time.minute();
}

void move_servo(uint16_t now)
{
  uint8_t servo_position = map(now, sun_position.sunrise.time, sun_position.sunset.time, Config::min_servo_pos, Config::max_servo_pos);
  Serial.print("servo pos: ");
  Serial.println(servo_position);

  m_servo.write(servo_position);
}

void set_sky_rgb(uint16_t now)
{
  const Color m_sky_blue = Color(0, 0, 12);
  uint32_t color = m_sky_blue.get_color();
  m_ws_leds.fill(color);
  m_ws_leds.show();
}

uint16_t sin_fun(float x, float max)
{
  return max * (sin(((x - max) * M_PI) / (max * 2))) + max;
}

uint16_t get_maped_time_to_color(uint16_t now, Point from, Point to, Colors color)
{
  switch (color)
  {
    case Colors::red:
      return map(now, from.time, to.time, from.color.r, to.color.r);
    case Colors::green:
      return map(now, from.time, to.time, from.color.g, to.color.g);
    case Colors::blue:
      return map(now, from.time, to.time, from.color.b, to.color.b);
    default:
      return 0;
  }
}

Color get_sun_horizon_rgb(uint16_t now, bool is_rising)
{
  Color color;
  if (is_rising)
  {
    Serial.println("rising under horizon");
    auto y = get_maped_time_to_color(now, sun_position.sunrise_civil, sun_position.sunrise, Colors::red);
    color.r = sin_fun(y, sun_position.sunrise.color.r);
    y = get_maped_time_to_color(now, sun_position.sunrise_civil, sun_position.sunrise, Colors::green);
    color.g = sin_fun(y, sun_position.sunrise.color.g);
    return color;
  }
  Serial.println("faling under horizon");
  auto y = get_maped_time_to_color(now, sun_position.sunset, sun_position.sunrise_civil, Colors::red);
  color.r = sin_fun(y, sun_position.sunset.color.r);
  y = get_maped_time_to_color(now, sun_position.sunset, sun_position.sunset_civil, Colors::green);
  color.g = sin_fun(y, sun_position.sunset.color.g);
  return color;
}

Color get_sun_day_rgb(uint16_t now, bool is_afternoon)
{
  Color color;
  if (is_afternoon)
  {
    Serial.println("rising to noon");
    auto y = get_maped_time_to_color(now, sun_position.sunrise, sun_position.noon, Colors::red);
    color.r = sin_fun(y, sun_position.noon.color.r);
    y = get_maped_time_to_color(now, sun_position.sunrise, sun_position.noon, Colors::green);
    color.g = sin_fun(y, sun_position.noon.color.g);
    return color;
  }
  Serial.println("faling to sunset");
  auto y = get_maped_time_to_color(now, sun_position.noon, sun_position.sunset, Colors::red);
  color.r = sin_fun(y, sun_position.noon.color.r);
  y = get_maped_time_to_color(now, sun_position.noon, sun_position.sunset, Colors::green);
  color.g = sin_fun(y, sun_position.noon.color.g);
  return color;
}

void check_day_part(uint16_t now)
{
  if (now > sun_position.sunset_civil.time)
  {
    actual_day_part = Day_part::night;
  }
  else if (now > sun_position.sunset.time)
  {
    actual_day_part = Day_part::sunset;
  }
  else if (now > sun_position.noon.time)
  {
    actual_day_part = Day_part::after_noon;
  }
  else if (now > sun_position.sunrise.time)
  {
    actual_day_part = Day_part::before_noon;
  }
  else if (now > sun_position.sunrise_civil.time)
  {
    actual_day_part = Day_part::sunrise;
  }
  else
  {
    actual_day_part = Day_part::night;
  }
}

void set_sun_rgb(uint16_t now)
{
  Color color;

  switch (actual_day_part)
  {
    case Day_part::night:
      color = sun_position.night.color;
      break;
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
  Serial.begin(9600);

  if (!m_rtc.begin())
  {
    Serial.println("Couldn't find RTC");
  }

  if (!m_rtc.isrunning())
  {
    Serial.println("RTC is NOT running, let's set the time!");
  }

  m_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  sun.setPosition(Config::latitude, Config::longitude, Config::dst_offset);
  sun.setCurrentDate(2022, 5, 5);

  pinMode(Config::pin_led_r, OUTPUT);
  pinMode(Config::pin_led_g, OUTPUT);
  pinMode(Config::pin_led_b, OUTPUT);

  m_ws_leds.begin();
  auto now = DateTime(2022, 5, 5, 12, 40, 0);
  auto minutes = calculate_from_datetime(now);
  set_sky_rgb(minutes);
  m_servo.write(0);
}

void loop()
{
  static unsigned long last_loop_time = 0;
  unsigned long loop_time = millis();
  if (loop_time - last_loop_time > m_refresh_time_ms)
  {
    // auto now = m_rtc.now();
    auto now = DateTime(2022, 5, 5, 12, 40, 0);

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

    check_day_part(minutes);

    set_sun_rgb(minutes);

    if (actual_day_part == Day_part::before_noon || actual_day_part == Day_part::after_noon)
    {
      move_servo(minutes);
    }

    if (actual_day_part == Day_part::sunrise || actual_day_part == Day_part::sunset)
    {
      set_sky_rgb(minutes);
    }
  }
}