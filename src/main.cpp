#include "Adafruit_NeoPixel.h"
#include "RTClib.h"
#include "math.h"
#include "sunset.h"

#include <Arduino.h>
#include <ArduinoSTL.h>
#include <Servo.h>
#include <Wire.h>


struct Color
{
  Color(uint8_t _r, uint8_t _g, uint8_t _b)
  : r(_r)
  , g(_g)
  , b(_b)
  {}
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint32_t get_color()
  {
    return Adafruit_NeoPixel::Color(r, g, b);
  }
};

struct Point
{
  Point(uint32_t _time, uint16_t _color)
  : time(_time)
  , color(_color)
  {}
  uint32_t time;
  uint16_t color;
}

const double m_latitude = 51.1078852;
const double m_longitude = 17.0385376;
const int8_t m_dst_offset = 2;

const unsigned long m_refresh_time_ms = 15000;

const byte m_pin_servo = 5;
const byte m_pin_led_r = 9;
const byte m_pin_led_g = 10;
const byte m_pin_led_b = 11;
const byte m_led_ws = 6;
const uint8_t m_led_ws_count = 10;
const uint8_t m_min_servo_pos = 0;
const uint8_t m_max_servo_pos = 180;

const uint8_t m_min_in_h = 60;

const Color m_sun_start = Color(0, 0, 0);
const Color m_sun_color = Color(255, 255, 51);
const Color m_sun_low_color = Color(242, 59, 28);

const Color m_sky_blue = Color(64, 166, 255);

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

void calculate_sunrise_sunset(uint16_t& sunrise, uint16_t& sunset, uint16_t& sunrise_civil, uint16_t& sunset_civil)
{
  sunrise = static_cast<uint16_t>(sun.calcSunrise());
  sunset = static_cast<uint16_t>(sun.calcSunset());
  sunrise_civil = static_cast<uint16_t>(sun.calcCivilSunrise());
  sunset_civil = static_cast<uint16_t>(sun.calcCivilSunset());
  Serial.print("Sunrise civil: ");
  print_time(calculate_from_minutes(sunrise_civil));
  Serial.print("Sunrise: ");
  print_time(calculate_from_minutes(sunrise));
  Serial.print("Sunset: ");
  print_time(calculate_from_minutes(sunset));
  Serial.print("Sunset civil: ");
  print_time(calculate_from_minutes(sunset_civil));
}

uint16_t calculate_from_datetime(DateTime time)
{
  return (time.hour() * 60) + time.minute();
}

void move_servo(uint16_t now, uint16_t sunrise, uint16_t sunset)
{
  if (!(now < sunrise))
  {
    uint8_t servo_position = map(now, sunrise, sunset, m_min_servo_pos, m_max_servo_pos);
    Serial.print("servo pos: ");
    Serial.println(servo_position);

    if (servo_position < m_max_servo_pos)
    {
      m_servo.write(servo_position);
    }
  }
}

void sky_led() {}

void Interpolation(std::vector<Point> points, uint16_t time)
{
  int j, i;
  float y, w1, w2, s;
  s = 0;
  w1 = 1;
  for (j = 1; j <= points.size(); j++)
  {
    w1 *= time - points[j].time;
    w2 = 1;
    for (i = 1; i <= points.size(); i++)

      if (i != j)
        w2 *= points[j].time - points[i].time;
    s += points[j].color / static_cast<float>(w2 * (time - points[j].time));
  }
  y = w1 * s;
}

void set_sun_rgb(uint16_t now, uint16_t start, uint16_t stop)
{
  // uint8_t r = map(now, start, stop, m_starting_pwm, m_sun_low_color.r);
  // uint8_t g = map(now, start, stop, m_starting_pwm, m_sun_low_color.r);
  // uint8_t b = map(now, start, stop, m_starting_pwm, m_sun_low_color.r);

  // Serial.print("sunrise r");
  // Serial.println(r);

  // analogWrite(m_pin_led_r, r);
  // analogWrite(m_pin_led_g, g);
  // analogWrite(m_pin_led_b, b);
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
  sun.setCurrentDate(2021, 10, 8);

  pinMode(m_pin_led_r, OUTPUT);
  pinMode(m_pin_led_g, OUTPUT);
  pinMode(m_pin_led_b, OUTPUT);

  m_ws_leds.begin();
}

void loop()
{
  static uint16_t sunrise = static_cast<uint16_t>(sun.calcSunrise());
  static uint16_t sunset = static_cast<uint16_t>(sun.calcSunset());
  static uint16_t sunrise_civil = static_cast<uint16_t>(sun.calcCivilSunrise());
  static uint16_t sunset_civil = static_cast<uint16_t>(sun.calcCivilSunset());

  static unsigned long last_loop_time = 0;
  unsigned long loop_time = millis();
  if (loop_time - last_loop_time > m_refresh_time_ms)
  {
    // auto now = m_rtc.now();
    auto now = DateTime(2021, 10, 8, 17, 45, 0);

    Serial.print("Now: ");
    print_time(now);
    last_loop_time = millis();

    Serial.print("Sunrise civil: ");
    print_time(calculate_from_minutes(sunrise_civil));
    Serial.print("Sunrise: ");
    print_time(calculate_from_minutes(sunrise));
    Serial.print("Sunset: ");
    print_time(calculate_from_minutes(sunset));
    Serial.print("Sunset civil: ");
    print_time(calculate_from_minutes(sunset_civil));

    auto minutes = calculate_from_datetime(now);

    move_servo(calculate_from_datetime(now), sunrise, sunset);

    // test
    analogWrite(m_pin_led_r, 255);
    // analogWrite(m_pin_led_r, test_servo * 2);
    // analogWrite(m_pin_led_b, test_servo /2);
    uint32_t color = m_sky_blue.get_color();
    m_ws_leds.fill(color);
    m_ws_leds.show();
  }
}