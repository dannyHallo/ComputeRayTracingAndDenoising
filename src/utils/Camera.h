#pragma once

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <vector>

#define W_BIT 1
#define A_BIT 2
#define S_BIT 4
#define D_BIT 8
#define SPACE_BIT 16
#define SHIFT_BIT 32
#define CTRL_BIT 64
#define ESC_BIT 128

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN, NONE };

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class Camera {
public:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 worldUp;

  // euler Angles
  float yaw, pitch;

  // camera options
  float movementSpeed    = .05f;
  float mouseSensitivity = 0.06f;
  float zoom             = 45;
  const float vFov       = 60;

  Camera(glm::vec3 camPosition = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 wrdUp = glm::vec3(0.0f, 1.0f, 0.0f), float camYaw = 180,
         float camPitch = 0)
      : position(camPosition), worldUp(wrdUp), yaw(camYaw), pitch(camPitch) {
    updateCameraVectors();
  }

  const glm::mat4 getViewMatrix() const { return glm::lookAt(position, position + front, up); }
  const glm::mat4 getProjectionMatrix(float aspectRatio, float zNear = 0.1f, float zFar = 10000) const;

  // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined
  // ENUM (to abstract it from windowing systems)
  void processKeyboard(CameraMovement direction, float deltaTime);

  // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
  void processMouseMovement(float xoffset, float yoffset);

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
  void processMouseScroll(float yoffset);

private:
  // calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors();
};
