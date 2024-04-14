#pragma once

struct CursorMoveInfo {
  float currentX;
  float currentY;
  float dx;
  float dy;
};

// dx and dy cannot be retrieved here
struct CursorInfo {
  float currentX;
  float currentY;
  bool leftButtonPressed;
  bool rightButtonPressed;
  bool middleButtonPressed;
};