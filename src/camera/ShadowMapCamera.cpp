#include "ShadowMapCamera.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

glm::vec3 constexpr kWorldUp             = {0.F, 1.F, 0.F};
float constexpr kShadowMapCameraDistance = 10.F;

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

glm::mat4 ShadowMapCamera::getProjectionMatrix() const { return glm::ortho(-1.F, 1.F, -1.F, 1.F, 0.1F, 10.F); }

// glm::mat4 ShadowMapCamera::getProjectionMatrix() const {
//   glm::mat4 projection =
//       glm::perspective(glm::radians(60.F), // The vertical Field of View, in radians: the amount
//                                             // of "zoom". Think "camera lens". Usually between
//                                             // 90° (extra wide) and 30° (quite zoomed in)
//                        1.F,
//                        0.1F, // Near clipping plane. Keep as big as possible, or you'll get
//                               // precision issues.
//                        10000.F   // Far clipping plane. Keep as little as possible.
//       );
//   return projection;
// }

void ShadowMapCamera::updateCameraVectors(glm::vec3 playerCameraPosition, glm::vec3 sunDir) {
  _position = sunDir * kShadowMapCameraDistance + playerCameraPosition;
  _front    = -sunDir;
  _right    = glm::normalize(glm::cross(_front, kWorldUp));
  _up       = glm::cross(_right, _front);
}
