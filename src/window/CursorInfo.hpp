#pragma once

struct CursorMoveInfo {
  bool firstMove  = true;
  double currentX = 0.F;
  double currentY = 0.F;
  double lastX    = 0.F;
  double lastY    = 0.F;
  double dx       = 0.F;
  double dy       = 0.F;
};

struct CursorInfo {
  CursorMoveInfo cursorMoveInfo = {};
  bool leftButtonPressed        = false;
  bool rightButtonPressed       = false;
  bool middleButtonPressed      = false;
};
