#include "ShadowMapCamera.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

glm::vec3 constexpr kWorldUp             = {0.F, 1.F, 0.F};
float constexpr kShadowMapCameraDistance = 10.F;

ShadowMapCamera::ShadowMapCamera(TomlConfigReader *tomlConfigReader)
    : _tomlConfigReader(tomlConfigReader) {
  _loadConfig();
}

ShadowMapCamera::~ShadowMapCamera() = default;

void ShadowMapCamera::_loadConfig() {
  _range = _tomlConfigReader->getConfig<float>("ShadowMapCamera.range");
}

glm::mat4 ShadowMapCamera::getProjectionMatrix() const {
  return glm::ortho(-_range, _range, -_range, _range, 0.1F, 10.F);
}

void ShadowMapCamera::updateCameraVectors(glm::vec3 playerCameraPosition, glm::vec3 sunDir) {
  _position = sunDir * kShadowMapCameraDistance + playerCameraPosition;
  _front    = -sunDir;
  _right    = glm::normalize(glm::cross(_front, kWorldUp));
  _up       = glm::cross(_right, _front);
}
