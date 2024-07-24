#include "ShadowMapCamera.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

glm::vec3 constexpr kWorldUp             = {0.F, 1.F, 0.F};
float constexpr kShadowMapCameraDistance = 1000.F;

ShadowMapCamera::ShadowMapCamera(TomlConfigReader *tomlConfigReader)
    : _tomlConfigReader(tomlConfigReader) {
  //   _loadConfig();
}

ShadowMapCamera::~ShadowMapCamera() = default;

// void ShadowMapCamera::_loadConfig() {
//   _position = glm::vec3(ps.at(0), ps.at(1), ps.at(2));
//   _yaw      = _tomlConfigReader->getConfig<float>("ShadowMapCamera.yaw");
//   _pitch    = _tomlConfigReader->getConfig<float>("ShadowMapCamera.pitch");
//   _vFov     = _tomlConfigReader->getConfig<float>("ShadowMapCamera.vFov");
// }

glm::mat4 ShadowMapCamera::getProjectionMatrix(float aspectRatio, float zNear, float zFar) const {
  return glm::ortho(0.F, 400.F, 0.F, 400.F);
}

void ShadowMapCamera::updateCameraVectors(glm::vec3 playerCameraPosition, glm::vec3 sunDir) {
  _position = sunDir * kShadowMapCameraDistance;
  _front    = -sunDir;
  _right    = glm::normalize(glm::cross(_front, kWorldUp));
  _up       = glm::cross(_right, _front);
}
