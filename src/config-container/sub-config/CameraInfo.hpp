#pragma once

class TomlConfigReader;

struct CameraInfo {
  float initHeight{};
  float initYaw{};   // in euler angles
  float initPitch{}; // in euler angles
  float vFov{};
  float movementSpeed{};
  float movementSpeedBoost{};
  float mouseSensitivity{};

  void loadConfig(TomlConfigReader *tomlConfigReader);
};
