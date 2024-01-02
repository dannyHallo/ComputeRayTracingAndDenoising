#include "Camera.hpp"

namespace {

constexpr float movementSpeed    = .05F;
constexpr float mouseSensitivity = 0.06F;

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

void Camera::processInput(float deltaTime) {
  if (_window->isInputBitActive(GLFW_KEY_ESCAPE)) {
    glfwSetWindowShouldClose(_window->getGlWindow(), 1);
    return;
  }

  if (_window->isInputBitActive(GLFW_KEY_TAB)) {
    _window->toggleCursor();
    _window->disableInputBit(GLFW_KEY_TAB);
    return;
  }

  processKeyboard(deltaTime);
}

void Camera::processKeyboard(float deltaTime) {
  if (!canMove()) {
    return;
  }

  constexpr float movementSpeedMultiplier = 10.F;

  float velocity = movementSpeedMultiplier * movementSpeed * deltaTime;

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
    _position += _up * velocity;
  }
  if (_window->isInputBitActive(GLFW_KEY_LEFT_SHIFT)) {
    _position -= _up * velocity;
  }
}

void Camera::handleMouseMovement(float xoffset, float yoffset) {
  if (!canMove()) {
    return;
  }

  xoffset *= -mouseSensitivity;
  yoffset *= mouseSensitivity;

  _yaw += xoffset;
  _pitch += yoffset;

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