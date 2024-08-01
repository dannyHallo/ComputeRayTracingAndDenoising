#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>                  // IWYU pragma: export
#include <glm/gtc/matrix_transform.hpp> // IWYU pragma: export
#include <glm/gtx/hash.hpp>             // IWYU pragma: export

struct ConfigContainer;

class ShadowMapCamera {
public:
  ShadowMapCamera(ConfigContainer *configContainer);
  ~ShadowMapCamera();

  // disable move and copy
  ShadowMapCamera(const ShadowMapCamera &)            = delete;
  ShadowMapCamera &operator=(const ShadowMapCamera &) = delete;
  ShadowMapCamera(ShadowMapCamera &&)                 = delete;
  ShadowMapCamera &operator=(ShadowMapCamera &&)      = delete;

  [[nodiscard]] glm::mat4 getViewMatrix() const {
    return glm::lookAt(_position, _position + _front, _up);
  }

  // processes input received from a mouse scroll-wheel event. Only requires
  // input on the vertical wheel-axis void processMouseScroll(float yoffset);
  [[nodiscard]] glm::mat4 getProjectionMatrix() const;

  [[nodiscard]] glm::vec3 getPosition() const { return _position; }
  [[nodiscard]] glm::vec3 getFront() const { return _front; }
  [[nodiscard]] glm::vec3 getUp() const { return _up; }
  [[nodiscard]] glm::vec3 getRight() const { return _right; }

  // calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors(glm::vec3 playerCameraPosition, glm::vec3 sunDir);

private:
  glm::vec3 _position = glm::vec3(0.F);

  glm::vec3 _front = glm::vec3(0.F);
  glm::vec3 _up    = glm::vec3(0.F);
  glm::vec3 _right = glm::vec3(0.F);

  ConfigContainer *_configContainer;
};
