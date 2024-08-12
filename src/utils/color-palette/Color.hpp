#pragma once

#include "imgui.h"

class Color {
public:
  Color() = default;
  Color(int r, int g, int b) : _r(r), _g(g), _b(b) {}

  [[nodiscard]] ImVec4 getImVec4() const { return {getR(), getG(), getB(), 1.F}; }

  [[nodiscard]] float getR() const { return static_cast<float>(_r) / 255.F; }
  [[nodiscard]] float getG() const { return static_cast<float>(_g) / 255.F; }
  [[nodiscard]] float getB() const { return static_cast<float>(_b) / 255.F; }

private:
  int _r;
  int _g;
  int _b;
};