#include "Camera.h"

namespace {
constexpr float movementSpeed    = .05F;
constexpr float mouseSensitivity = 0.06F;
// constexpr float zoom             = 45;
} // namespace

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float zNear,
                                      float zFar) const {
  glm::mat4 projection = glm::perspective(
      glm::radians(mVFov), // The vertical Field of View, in radians: the amount
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
  if (mWindow->isInputBitActive(GLFW_KEY_ESCAPE)) {
    glfwSetWindowShouldClose(mWindow->getGlWindow(), 1);
    return;
  }

  if (mWindow->isInputBitActive(GLFW_KEY_TAB)) {
    mWindow->toggleCursor();
    mWindow->disableInputBit(GLFW_KEY_TAB);
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

  if (mWindow->isInputBitActive(GLFW_KEY_W)) {
    mPosition += mFront * velocity;
  }
  if (mWindow->isInputBitActive(GLFW_KEY_S)) {
    mPosition -= mFront * velocity;
  }
  if (mWindow->isInputBitActive(GLFW_KEY_A)) {
    mPosition -= mRight * velocity;
  }
  if (mWindow->isInputBitActive(GLFW_KEY_D)) {
    mPosition += mRight * velocity;
  }
  if (mWindow->isInputBitActive(GLFW_KEY_SPACE)) {
    mPosition += mUp * velocity;
  }
  if (mWindow->isInputBitActive(GLFW_KEY_LEFT_SHIFT)) {
    mPosition -= mUp * velocity;
  }
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
  if (!canMove()) {
    return;
  }

  xoffset *= -mouseSensitivity;
  yoffset *= mouseSensitivity;

  mYaw += xoffset;
  mPitch += yoffset;

  constexpr float cameraLim = 89.9F;
  // make sure that when mPitch is out of bounds, screen doesn't get flipped
  if (mPitch > cameraLim) {
    mPitch = cameraLim;
  }
  if (mPitch < -cameraLim) {
    mPitch = -cameraLim;
  }

  // update Front, Right and Up Vectors using the updated Euler angles
  updateCameraVectors();
}

void Camera::updateCameraVectors() {
  mFront = {-sin(glm::radians(mYaw)) * cos(glm::radians(mPitch)),
            sin(glm::radians(mPitch)),
            -cos(glm::radians(mYaw)) * cos(glm::radians(mPitch))};
  // normalize the vectors, because their length gets closer to 0 the
  mRight = glm::normalize(glm::cross(mFront, mWorldUp));
  // more you look up or down which results in slower movement.
  mUp = glm::cross(mRight, mFront);
}