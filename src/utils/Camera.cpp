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

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
  float velocity = 10.f * movementSpeed * deltaTime;
  if (direction == FORWARD)
    position += front * velocity;
  if (direction == BACKWARD)
    position -= front * velocity;
  if (direction == LEFT)
    position -= right * velocity;
  if (direction == RIGHT)
    position += right * velocity;
  if (direction == UP)
    position += up * velocity;
  if (direction == DOWN)
    position -= up * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
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