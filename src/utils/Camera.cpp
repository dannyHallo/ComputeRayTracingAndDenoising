#include "Camera.h"

const glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float zNear, float zFar) const {
  glm::mat4 projection =
      glm::perspective(glm::radians(vFov), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens".
                                           // Usually between 90° (extra wide) and 30° (quite zoomed in)
                       aspectRatio,
                       zNear, // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                       zFar   // Far clipping plane. Keep as little as possible.
      );
  return projection;
}

void Camera::processInput(float deltaTime) {
  uint32_t inputBits = mWindow->getKeyInputs();

  if (inputBits & ESC_BIT)
    glfwSetWindowShouldClose(mWindow->getWindow(), true);
  if (inputBits & TAB_BIT) {
    mWindow->toggleCursor();
    mWindow->disableInputBit(TAB_BIT);
  }

  if (inputBits & (SPACE_BIT | SHIFT_BIT | W_BIT | S_BIT | A_BIT | D_BIT))
    processKeyboard(inputBits, deltaTime);
}

void Camera::processKeyboard(uint32_t inputBits, float deltaTime) {
  if (!canMove())
    return;

  float velocity = 10.f * movementSpeed * deltaTime;
  if (inputBits & W_BIT)
    position += front * velocity;
  if (inputBits & S_BIT)
    position -= front * velocity;
  if (inputBits & A_BIT)
    position -= right * velocity;
  if (inputBits & D_BIT)
    position += right * velocity;
  if (inputBits & SPACE_BIT)
    position += up * velocity;
  if (inputBits & SHIFT_BIT)
    position -= up * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
  if (!canMove())
    return;
  xoffset *= -mouseSensitivity;
  yoffset *= mouseSensitivity;

  yaw += xoffset;
  pitch += yoffset;

  // make sure that when pitch is out of bounds, screen doesn't get flipped
  if (pitch > 89.0f)
    pitch = 89.0f;
  if (pitch < -89.0f)
    pitch = -89.0f;

  // update Front, Right and Up Vectors using the updated Euler angles
  updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
  if (!canMove())
    return;
  zoom -= (float)yoffset;
  if (zoom < 1.0f)
    zoom = 1.0f;
  if (zoom > 45.0f)
    zoom = 45.0f;
}

void Camera::updateCameraVectors() {
  front.x = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));

  // normalize the vectors, because their length gets closer to 0 the
  right = glm::normalize(glm::cross(front, worldUp));
  // more you look up or down which results in slower movement.
  up = glm::cross(right, front);
}