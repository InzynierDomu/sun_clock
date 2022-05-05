#include "Adafruit_NeoPixel.h"
#include "RTClib.h"
#include "math.h"
#include "sunset.h"

#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <math.h>
#include <stdint.h>

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

const double m_latitude = 51.1078852;
const double m_longitude = 17.0385376;
const int8_t m_dst_offset = 2;

const unsigned long m_refresh_time_ms = 15000;

const byte m_pin_servo = 9;
const byte m_pin_led_r = 3;
const byte m_pin_led_g = 5;
const byte m_pin_led_b = 6;
const byte m_led_ws = 7;
const uint8_t m_led_ws_count = 10;
const uint8_t m_min_servo_pos = 0;
const uint8_t m_max_servo_pos = 180;

const uint8_t m_min_in_h = 60;

Sun_position sun_position;

const Color m_sky_blue = Color(0, 0, 10);

RTC_DS1307 m_rtc; ///< DS1307 RTC
SunSet sun;
Servo m_servo;
Adafruit_NeoPixel m_ws_leds(m_led_ws_count, m_led_ws, NEO_GRB + NEO_KHZ800);

DateTime calculate_from_minutes(uint16_t total_min)
{
  m_servo.attach(m_pin_servo);

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
  sun_position.noon = Point(day_middle, Color(255, 255, 0));
  sun_position.sunset = Point(sunset, Color(27, 2, 0));
  sun_position.sunset_civil = Point(sunset_civil, Color());
}

uint16_t calculate_from_datetime(DateTime time)
{
  return (time.hour() * 60) + time.minute();
}

void move_servo(uint16_t now)
{
  if (now > sun_position.sunrise.time)
  {
    uint8_t servo_position = map(now, sun_position.sunrise.time, sun_position.sunset.time, m_min_servo_pos, m_max_servo_pos);
    Serial.print("servo pos: ");
    Serial.println(servo_position);

    if (servo_position < m_max_servo_pos)
    {
      m_servo.write(servo_position);
    }
  }
  m_servo.write(0);
}

enum class colors
{
  red,
  green,
  blue
};

void set_sky_rgb(uint16_t now)
{
  // todo: rising with sunset and falling wiht sunset
  uint32_t color = m_sky_blue.get_color();
  m_ws_leds.fill(color);
  m_ws_leds.show();
}

uint16_t sin_fun(float x, float max)
{
  return max * (sin(((x - max) * M_PI) / (max * 2))) + max;
}

uint16_t get_maped_time_to_color(uint16_t now, Point from, Point to, colors color)
{
  switch (color)
  {
    case colors::red:
      return map(now, from.time, to.time, from.color.r, to.color.r);
    case colors::green:
      return map(now, from.time, to.time, from.color.g, to.color.g);
    case colors::blue:
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
    auto y = get_maped_time_to_color(now, sun_position.sunrise_civil, sun_position.sunrise, colors::red);
    color.r = sin_fun(y, sun_position.sunrise.color.r);
    y = get_maped_time_to_color(now, sun_position.sunrise_civil, sun_position.sunrise, colors::green);
    color.g = sin_fun(y, sun_position.sunrise.color.g);
    return color;
  }
  Serial.println("faling under horizon");
  auto y = get_maped_time_to_color(now, sun_position.sunset, sun_position.sunrise_civil, colors::red);
  color.r = sin_fun(y, sun_position.sunset.color.r);
  y = get_maped_time_to_color(now, sun_position.sunset, sun_position.sunset_civil, colors::green);
  color.g = sin_fun(y, sun_position.sunset.color.g);
  return color;
}

Color get_sun_day_rgb(uint16_t now, bool is_afternoon)
{
  Color color;
  if (is_afternoon)
  {
    Serial.println("rising to noon");
    auto y = get_maped_time_to_color(now, sun_position.sunrise, sun_position.noon, colors::red);
    color.r = sin_fun(y, sun_position.sunrise.noon.r);
    y = get_maped_time_to_color(now, sun_position.sunrise, sun_position.noon, colors::green);
    color.g = sin_fun(y, sun_position.sunrise.noon.g);
    return color;
  }
  Serial.println("faling to sunset");
  auto y = get_maped_time_to_color(now, sun_position.noon, sun_position.sunset, colors::red);
  color.r = sin_fun(y, sun_position.sunrise.noon.r);
  y = get_maped_time_to_color(now, sun_position.noon, sun_position.sunset, colors::green);
  color.g = sin_fun(y, sun_position.sunrise.noon.g);
  return color;
}

void set_sun_rgb(uint16_t now)
{
  // todo: make some state machine - reuse in sky color set
  Color color;
  if (now > sun_position.sunset_civil.time)
  {
    color = sun_position.night.color;
    Serial.println("night");
  }
  else if (now > sun_position.sunset.time)
  {
    color = get_sun_horizon_rgb(now, false);
  }
  else if (now > sun_position.noon.time)
  {
    color = get_sun_day_rgb(now, false);
  }
  else if (now > sun_position.sunrise.time)
  {
    color = get_sun_day_rgb(now, true);
  }
  else if (now > sun_position.sunrise_civil.time)
  {
    color = get_sun_horizon_rgb(now, true);
  }
  else
  {
    color = sun_position.night.color;
    Serial.println("night");
  }

  analogWrite(m_pin_led_r, color.r);
  analogWrite(m_pin_led_g, color.g);
  analogWrite(m_pin_led_b, color.b);
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

  sun.setPosition(m_latitude, m_longitude, m_dst_offset);
  sun.setCurrentDate(2022, 5, 5);

  pinMode(m_pin_led_r, OUTPUT);
  pinMode(m_pin_led_g, OUTPUT);
  pinMode(m_pin_led_b, OUTPUT);

  m_ws_leds.begin();
}

void loop()
{
  static unsigned long last_loop_time = 0;
  unsigned long loop_time = millis();
  if (loop_time - last_loop_time > m_refresh_time_ms)
  {
    // auto now = m_rtc.now();
    auto now = DateTime(2022, 5, 5, 20, 40, 0);

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

    set_sun_rgb(minutes);

    move_servo(minutes);

    set_sky_rgb(minutes);
  }
}