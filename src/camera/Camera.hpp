#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>                  // IWYU pragma: export
#include <glm/gtc/matrix_transform.hpp> // IWYU pragma: export
#include <glm/gtx/hash.hpp>             // IWYU pragma: export

#include "window/CursorInfo.hpp"
#include "window/Window.hpp"

class TomlConfigReader;

class Camera {
public:
  Camera(Window *window, TomlConfigReader *tomlConfigReader);
  ~Camera();

  // disable move and copy
  Camera(const Camera &)            = delete;
  Camera &operator=(const Camera &) = delete;
  Camera(Camera &&)                 = delete;
  Camera &operator=(Camera &&)      = delete;

  [[nodiscard]] glm::mat4 getViewMatrix() const {
    return glm::lookAt(_position, _position + _front, _up);
  }

  void processInput(double deltaTime);

  // processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems)
  void processKeyboard(double deltaTime);

  // processes input received from a mouse input system. Expects the offset
  // value in both the x and y direction.
  void handleMouseMovement(CursorMoveInfo const &mouseInfo);

  // processes input received from a mouse scroll-wheel event. Only requires
  // input on the vertical wheel-axis void processMouseScroll(float yoffset);
  [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio, float zNear = 0.1F,
                                              float zFar = 10000.F) const;

  [[nodiscard]] glm::vec3 getPosition() const { return _position; }
  [[nodiscard]] glm::vec3 getFront() const { return _front; }
  [[nodiscard]] glm::vec3 getUp() const { return _up; }
  [[nodiscard]] glm::vec3 getRight() const { return _right; }
  [[nodiscard]] float getVFov() const { return _vFov; }

private:
  // config
  glm::vec3 _position;
  float _yaw;   // in euler angles
  float _pitch; // in euler angles
  float _vFov;
  float _movementSpeed;
  float _movementSpeedBoost;
  float _mouseSensitivity;
  void _loadConfig();

  glm::vec3 _front = glm::vec3(0.F);
  glm::vec3 _up    = glm::vec3(0.F);
  glm::vec3 _right = glm::vec3(0.F);

  // window is owned by the Application class
  Window *_window;
  TomlConfigReader *_tomlConfigReader;

  float _movementSpeedMultiplier = 1.F;

  // calculates the front vector from the Camera's (updated) Euler Angles
  void _updateCameraVectors();
  [[nodiscard]] bool canMove() const {
    return _window->getCursorState() == CursorState::kInvisible;
  }
};
