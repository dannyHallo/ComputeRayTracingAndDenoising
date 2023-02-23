#pragma once

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "utils/debug.h"

#include <iostream>
#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN, NONE };

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

  Camera(glm::vec3 camPosition = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 wrdUp = glm::vec3(0.0f, 1.0f, 0.0f),
         float camYaw = 180, float camPitch = 0)
      : position(camPosition), worldUp(wrdUp), yaw(camYaw), pitch(camPitch) {
    updateCameraVectors();
  }

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 getViewMatrix() { return glm::lookAt(position, position + front, up); }

  glm::mat4 getProjectionMatrix(float aspectRatio, float zNear = 0.1f, float zFar = 10000) {
    // glm::mat4 view = glm::mat4(1.0f);

    // zNear = -zNear;
    // zFar  = -zFar;

    // float a = 2 * tan(glm::radians(vFov) / 2.f) * fabs(zNear);
    // float b = a * aspectRatio;

    // glm::mat4 projection{}; // initiated to indentity matrix

    // glm::mat4 perspToOrtho{zNear, 0, 0, 0, 0, zNear, 0, 0, 0, 0, zNear + zFar, -zNear * zFar, 0, 0, 1.f, 0};

    // glm::mat4 translation{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -(zNear + zFar) / 2.f, 0, 0, 0, 1};

    // glm::mat4 scaling{2.f / a, 0, 0, 0, 0, 2.f / b, 0, 0, 0, 0, 2.f / (zNear - zFar), 0, 0, 0, 0, 1.f};

    // projection = scaling * translation * perspToOrtho;

    // std::cout << glm::to_string(projection); TODO:
    
    glm::mat4 projection2 = glm::perspective(
        glm::radians(vFov), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens".
                            // Usually between 90° (extra wide) and 30° (quite zoomed in)
        aspectRatio, 
        zNear,       // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        zFar         // Far clipping plane. Keep as little as possible.
    );
    // std::cout << glm::to_string(projection2);
    return projection2;
  }

  // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined
  // ENUM (to abstract it from windowing systems)
  void processKeyboard(Camera_Movement direction, float deltaTime) {
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

    // print(position);
  }

  // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
  void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
    xoffset *= -mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
      if (pitch > 89.0f)
        pitch = 89.0f;
      if (pitch < -89.0f)
        pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
  }

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
  void processMouseScroll(float yoffset) {
    zoom -= (float)yoffset;
    if (zoom < 1.0f)
      zoom = 1.0f;
    if (zoom > 45.0f)
      zoom = 45.0f;
  }

private:
  // calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors() {
    front.x = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));

    // normalize the vectors, because their length gets closer to 0 the
    right = glm::normalize(glm::cross(front, worldUp));
    // more you look up or down which results in slower movement.
    up = glm::cross(right, front);
  }
};
