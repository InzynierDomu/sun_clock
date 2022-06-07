/**
 * @file Color.h
 * @brief Struct for color
 * @author by Szymon Markiewicz
 * @details http://www.inzynierdomu.pl/
 * @date 05-2022
 */

#pragma once

#include <stdint.h>

///< colors
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
  uint8_t r; ///< red saturation
  uint8_t g; ///< green saturation
  uint8_t b; ///< blue saturation
  /**
   * @brief Get the color object
   * @return uint32_t
   */
  uint32_t get_color() const
  {
    uint32_t color = b;
    color += g << 8;
    color += r << 16;
    return color;
  }
};