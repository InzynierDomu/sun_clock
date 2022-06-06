/**
 * @file Color.h
 * @brief Struct for color
 * @author by Szymon Markiewicz
 * @details http://www.inzynierdomu.pl/
 * @date 05-2022
 */

#pragma once

#include <stdint.h>

enum class Colors
{
  red,
  green,
  blue
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