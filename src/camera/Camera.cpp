#include "Camera.hpp"

namespace {

float constexpr kMovementSpeed    = .2F;
float constexpr kMouseSensitivity = 0.06F;

} // namespace

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float zNear, float zFar) const {
  glm::mat4 projection =
      glm::perspective(glm::radians(_fov), // The vertical Field of View, in radians: the amount
                                           // of "zoom". Think "camera lens". Usually between
                                           // 90° (extra wide) and 30° (quite zoomed in)
                       aspectRatio,
                       zNear, // Near clipping plane. Keep as big as possible, or you'll get
                              // precision issues.
                       zFar   // Far clipping plane. Keep as little as possible.
      );
  return projection;
}

void Camera::processInput(double deltaTime) {
  if (_window->isInputBitActive(GLFW_KEY_ESCAPE)) {
    glfwSetWindowShouldClose(_window->getGlWindow(), 1);
    return;
  }

  if (_window->isInputBitActive(GLFW_KEY_E)) {
    _window->toggleCursor();
    _window->disableInputBit(GLFW_KEY_E);
    return;
  }

  processKeyboard(deltaTime);
}

void Camera::processKeyboard(double deltaTime) {
  if (!canMove()) {
    return;
  }

  float velocity = _movementSpeedMultiplier * kMovementSpeed * static_cast<float>(deltaTime);

  if (_window->isInputBitActive(GLFW_KEY_W)) {
    _position += _front * velocity;
  }
  if (_window->isInputBitActive(GLFW_KEY_S)) {
    _position -= _front * velocity;
  }
  if (_window->isInputBitActive(GLFW_KEY_A)) {
    _position -= _right * velocity;
  }
  if (_window->isInputBitActive(GLFW_KEY_D)) {
    _position += _right * velocity;
  }
  if (_window->isInputBitActive(GLFW_KEY_SPACE)) {
    _position += _worldUp * velocity;
  }
  if (_window->isInputBitActive(GLFW_KEY_LEFT_SHIFT)) {
    _movementSpeedMultiplier = 2.F;
  } else {
    _movementSpeedMultiplier = 1.F;
  }
  if (_window->isInputBitActive(GLFW_KEY_LEFT_CONTROL)) {
    _position -= _worldUp * velocity;
  }
}

void Camera::handleMouseMovement(MouseInfo const &mouseInfo) {
  if (!canMove()) {
    return;
  }

  float mouseDx = mouseInfo.dx;
  float mouseDy = mouseInfo.dy;

  mouseDx *= -kMouseSensitivity;
  mouseDy *= kMouseSensitivity;

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
  _right = glm::normalize(glm::cross(_front, _worldUp));
  // more you look up or down which results in slower movement.
  _up = glm::cross(_right, _front);
}