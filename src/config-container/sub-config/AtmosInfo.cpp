#include "AtmosInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void AtmosInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  sunAltitude = tomlConfigReader->getConfig<float>("AtmosInfo.sunAltitude");
  sunAzimuth  = tomlConfigReader->getConfig<float>("AtmosInfo.sunAzimuth");
  auto const &rsb =
      tomlConfigReader->getConfig<std::array<float, 3>>("AtmosInfo.rayleighScatteringBase");
  rayleighScatteringBase = glm::vec3(rsb.at(0), rsb.at(1), rsb.at(2));
  mieScatteringBase      = tomlConfigReader->getConfig<float>("AtmosInfo.mieScatteringBase");
  mieAbsorptionBase      = tomlConfigReader->getConfig<float>("AtmosInfo.mieAbsorptionBase");
  auto const &oab =
      tomlConfigReader->getConfig<std::array<float, 3>>("AtmosInfo.ozoneAbsorptionBase");
  ozoneAbsorptionBase = glm::vec3(oab.at(0), oab.at(1), oab.at(2));
  sunLuminance        = tomlConfigReader->getConfig<float>("AtmosInfo.sunLuminance");
  atmosLuminance      = tomlConfigReader->getConfig<float>("AtmosInfo.atmosLuminance");
  sunSize             = tomlConfigReader->getConfig<float>("AtmosInfo.sunSize");
  rayleighDropoffFac  = tomlConfigReader->getConfig<float>("AtmosInfo.rayleighDropoffFac");
  mieDropoffFac       = tomlConfigReader->getConfig<float>("AtmosInfo.mieDropoffFac");
}
