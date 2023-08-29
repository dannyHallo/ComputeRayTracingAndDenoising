#pragma once

#include "Window/Window.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <vector>

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class Camera {
  glm::vec3 mPosition;
  glm::vec3 mFront;
  glm::vec3 mUp;
  glm::vec3 mRight;
  glm::vec3 mWorldUp;

  // euler Angles
  float yaw;
  float pitch;
  float vFov;

  // window is owned by ApplicationContext
  Window *mWindow;

public:
  Camera(glm::vec3 camPosition = glm::vec3(0.F, 0.F, 0.F), glm::vec3 wrdUp = glm::vec3(0.F, 1.F, 0.F),
         float camYaw = 180, float camPitch = 0)
      : mPosition(camPosition), mWorldUp(wrdUp), yaw(camYaw), pitch(camPitch) {
    updateCameraVectors();
  }

  void init(Window *window) { mWindow = window; }
  [[nodiscard]] glm::mat4 getViewMatrix() const { return glm::lookAt(mPosition, mPosition + mFront, mUp); }

  void processInput(float deltaTime);

  // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined
  // ENUM (to abstract it from windowing systems)
  void processKeyboard(uint32_t inputBits, float deltaTime);

  // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
  void processMouseMovement(float xoffset, float yoffset);

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
  // void processMouseScroll(float yoffset);
  [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio, float zNear = 0.1F, float zFar = 10000) const;

  [[nodiscard]] glm::vec3 getPosition() const { return mPosition; }
  [[nodiscard]] glm::vec3 getFront() const { return mFront; }
  [[nodiscard]] glm::vec3 getUp() const { return mUp; }
  [[nodiscard]] glm::vec3 getRight() const { return mRight; }
  [[nodiscard]] float getVFov() const { return vFov; }

private:
  // calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors();
  [[nodiscard]] bool canMove() const { return mWindow->getCursorState() == CursorState::INVISIBLE; }
};
