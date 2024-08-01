#include "ShadowMapCamera.hpp"

#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/ShadowMapCameraInfo.hpp"

glm::vec3 constexpr kWorldUp             = {0.F, 1.F, 0.F};
float constexpr kShadowMapCameraDistance = 10.F;

ShadowMapCamera::ShadowMapCamera(ConfigContainer *configContainer)
    : _configContainer(configContainer) {}

ShadowMapCamera::~ShadowMapCamera() = default;

glm::mat4 ShadowMapCamera::getProjectionMatrix() const {
  auto const range = _configContainer->shadowMapCameraInfo->range;
  return glm::ortho(-range, range, -range, range, 0.1F, 10.F);
}

void ShadowMapCamera::updateCameraVectors(glm::vec3 playerCameraPosition, glm::vec3 sunDir) {
  _position = sunDir * kShadowMapCameraDistance + playerCameraPosition;
  _front    = -sunDir;
  _right    = glm::normalize(glm::cross(_front, kWorldUp));
  _up       = glm::cross(_right, _front);
}
