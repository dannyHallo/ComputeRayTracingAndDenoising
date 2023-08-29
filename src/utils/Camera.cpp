#include "Camera.h"

namespace {
constexpr float movementSpeed    = .05F;
constexpr float mouseSensitivity = 0.06F;
// constexpr float zoom             = 45;
} // namespace

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float zNear, float zFar) const {
  glm::mat4 projection =
      glm::perspective(glm::radians(vFov), // The vertical Field of View, in radians: the amount of "zoom". Think
                                           // "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
                       aspectRatio,
                       zNear, // Near clipping plane. Keep as big as possible, or you'll get precision issues.
                       zFar   // Far clipping plane. Keep as little as possible.
      );
  return projection;
}

void Camera::processInput(float deltaTime) {
  uint32_t inputBits = mWindow->getKeyInputs();

  if ((inputBits & ESC_BIT) != 0) {
    glfwSetWindowShouldClose(mWindow->getWindow(), 1);
  }

  if ((inputBits & TAB_BIT) != 0) {
    mWindow->toggleCursor();
    mWindow->disableInputBit(TAB_BIT);
  }

  if ((inputBits & (SPACE_BIT | SHIFT_BIT | W_BIT | S_BIT | A_BIT | D_BIT)) != 0) processKeyboard(inputBits, deltaTime);
}

void Camera::processKeyboard(uint32_t inputBits, float deltaTime) {
  if (!canMove()) {
    return;
  }

  constexpr float movementSpeedMultiplier = 10.F;

  float velocity = movementSpeedMultiplier * movementSpeed * deltaTime;

  if ((inputBits & W_BIT) != 0) {
    mPosition += mFront * velocity;
  }
  if ((inputBits & S_BIT) != 0) {
    mPosition -= mFront * velocity;
  }
  if ((inputBits & A_BIT) != 0) {
    mPosition -= mRight * velocity;
  }
  if ((inputBits & D_BIT) != 0) {
    mPosition += mRight * velocity;
  }
  if ((inputBits & SPACE_BIT) != 0) {
    mPosition += mUp * velocity;
  }
  if ((inputBits & SHIFT_BIT) != 0) {
    mPosition -= mUp * velocity;
  }
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
  if (!canMove()) {
    return;
  }

  xoffset *= -mouseSensitivity;
  yoffset *= mouseSensitivity;

  yaw += xoffset;
  pitch += yoffset;

  constexpr float cameraLim = 89.9F;
  // make sure that when pitch is out of bounds, screen doesn't get flipped
  if (pitch > cameraLim) {
    pitch = cameraLim;
  }
  if (pitch < -cameraLim) {
    pitch = -cameraLim;
  }

  // update Front, Right and Up Vectors using the updated Euler angles
  updateCameraVectors();
}

// void Camera::processMouseScroll(float yoffset) {
//   if (!canMove()) {
//     return;
//   }

// zoom -= (float)yoffset;
// if (zoom < 1.0f) zoom = 1.0f;
// if (zoom > 45.0f) zoom = 45.0f;
// }

void Camera::updateCameraVectors() {
  mFront = {-sin(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)),
            -cos(glm::radians(yaw)) * cos(glm::radians(pitch))};
  // normalize the vectors, because their length gets closer to 0 the
  mRight = glm::normalize(glm::cross(mFront, mWorldUp));
  // more you look up or down which results in slower movement.
  mUp = glm::cross(mRight, mFront);
}