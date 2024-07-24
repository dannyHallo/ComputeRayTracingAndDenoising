#include "Camera.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"
#include "window/KeyboardInfo.hpp"

glm::vec3 constexpr kWorldUp = {0.F, 1.F, 0.F};

Camera::Camera(Window *window, TomlConfigReader *tomlConfigReader)
    : _window(window), _tomlConfigReader(tomlConfigReader) {
  _loadConfig();
  _updateCameraVectors();
}

Camera::~Camera() = default;

void Camera::_loadConfig() {
  auto ps             = _tomlConfigReader->getConfig<std::array<float, 3>>("Camera.position");
  _position           = glm::vec3(ps.at(0), ps.at(1), ps.at(2));
  _yaw                = _tomlConfigReader->getConfig<float>("Camera.yaw");
  _pitch              = _tomlConfigReader->getConfig<float>("Camera.pitch");
  _vFov               = _tomlConfigReader->getConfig<float>("Camera.vFov");
  _movementSpeed      = _tomlConfigReader->getConfig<float>("Camera.movementSpeed");
  _movementSpeedBoost = _tomlConfigReader->getConfig<float>("Camera.movementSpeedBoost");
  _mouseSensitivity   = _tomlConfigReader->getConfig<float>("Camera.mouseSensitivity");
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float zNear, float zFar) const {
  glm::mat4 projection =
      glm::perspective(glm::radians(_vFov), // The vertical Field of View, in radians: the amount
                                            // of "zoom". Think "camera lens". Usually between
                                            // 90° (extra wide) and 30° (quite zoomed in)
                       aspectRatio,
                       zNear, // Near clipping plane. Keep as big as possible, or you'll get
                              // precision issues.
                       zFar   // Far clipping plane. Keep as little as possible.
      );
  return projection;
}

void Camera::processInput(double deltaTime) { processKeyboard(deltaTime); }

void Camera::processKeyboard(double deltaTime) {
  if (!canMove()) {
    return;
  }

  float velocity = _movementSpeedMultiplier * _movementSpeed * static_cast<float>(deltaTime);

  KeyboardInfo const &ki = _window->getKeyboardInfo();

  if (ki.isKeyPressed(GLFW_KEY_W)) {
    _position += _front * velocity;
  }
  if (ki.isKeyPressed(GLFW_KEY_S)) {
    _position -= _front * velocity;
  }
  if (ki.isKeyPressed(GLFW_KEY_A)) {
    _position -= _right * velocity;
  }
  if (ki.isKeyPressed(GLFW_KEY_D)) {
    _position += _right * velocity;
  }
  if (ki.isKeyPressed(GLFW_KEY_SPACE)) {
    _position += kWorldUp * velocity;
  }
  if (ki.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
    _movementSpeedMultiplier = _movementSpeedBoost;
  } else {
    _movementSpeedMultiplier = 1.F;
  }
  if (ki.isKeyPressed(GLFW_KEY_LEFT_CONTROL)) {
    _position -= kWorldUp * velocity;
  }
}

void Camera::handleMouseMovement(CursorMoveInfo const &mouseInfo) {
  if (!canMove()) {
    return;
  }

  float mouseDx = mouseInfo.dx;
  float mouseDy = mouseInfo.dy;

  mouseDx *= -_mouseSensitivity;
  mouseDy *= _mouseSensitivity;

  _yaw += mouseDx;
  _pitch += mouseDy;

  constexpr float cameraLim = 89.9F;
  // make sure that when mPitch is out of bounds, screen doesn't get flipped
  if (_pitch > cameraLim) {
    _pitch = cameraLim;
  }
  if (_pitch < -cameraLim) {
    _pitch = -cameraLim;
  }

  // update Front, Right and Up Vectors using the updated Euler angles
  _updateCameraVectors();
}

void Camera::_updateCameraVectors() {
  _front = {-sin(glm::radians(_yaw)) * cos(glm::radians(_pitch)), sin(glm::radians(_pitch)),
            -cos(glm::radians(_yaw)) * cos(glm::radians(_pitch))};
  // normalize the vectors, because their length gets closer to 0 the
  _right = glm::normalize(glm::cross(_front, kWorldUp));
  // more you look up or down which results in slower movement.
  _up = glm::cross(_right, _front);
}
