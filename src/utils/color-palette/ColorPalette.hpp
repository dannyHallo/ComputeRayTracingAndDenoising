#pragma once

#include "Color.hpp"

#include <map>
#include <string>

class ColorPalette {
public:
  ColorPalette()  = default;
  ~ColorPalette() = default;

  // delete copy and move
  ColorPalette(const ColorPalette &)            = delete;
  ColorPalette(ColorPalette &&)                 = delete;
  ColorPalette &operator=(const ColorPalette &) = delete;
  ColorPalette &operator=(ColorPalette &&)      = delete;

  void addColor(std::string const &name, Color const &color) {
    _palettes.insert(std::make_pair(name, color));
  }

  [[nodiscard]] Color getColorByName(std::string const &name) const { return _palettes.at(name); }

private:
  std::map<std::string, Color> _palettes;
};
