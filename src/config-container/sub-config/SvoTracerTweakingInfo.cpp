#include "SvoTracerTweakingInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void SvoTracerTweakingInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  debugB1 = tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.debugB1");
  debugF1 = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.debugF1");
  debugI1 = tomlConfigReader->getConfig<int>("SvoTracerTweakingData.debugI1");
  auto const &dc1 =
      tomlConfigReader->getConfig<std::array<float, 3>>("SvoTracerTweakingData.debugC1");
  debugC1 = glm::vec3(dc1.at(0), dc1.at(1), dc1.at(2));

  explosure = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.explosure");

  visualizeChunks  = tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.visualizeChunks");
  visualizeOctree  = tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.visualizeOctree");
  beamOptimization = tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.beamOptimization");
  traceIndirectRay = tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.traceIndirectRay");
  taa              = tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.taa");

  sunAltitude     = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunAltitude");
  sunAzimuth      = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunAzimuth");
  auto const &rsb = tomlConfigReader->getConfig<std::array<float, 3>>(
      "SvoTracerTweakingData.rayleighScatteringBase");
  rayleighScatteringBase = glm::vec3(rsb.at(0), rsb.at(1), rsb.at(2));
  mieScatteringBase = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.mieScatteringBase");
  mieAbsorptionBase = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.mieAbsorptionBase");
  auto const &oab   = tomlConfigReader->getConfig<std::array<float, 3>>(
      "SvoTracerTweakingData.ozoneAbsorptionBase");
  ozoneAbsorptionBase = glm::vec3(oab.at(0), oab.at(1), oab.at(2));
  sunLuminance        = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunLuminance");
  atmosLuminance      = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.atmosLuminance");
  sunSize             = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunSize");

  temporalAlpha = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.temporalAlpha");
  temporalPositionPhi =
      tomlConfigReader->getConfig<float>("SvoTracerTweakingData.temporalPositionPhi");

  aTrousIterationCount =
      tomlConfigReader->getConfig<int>("SvoTracerTweakingData.aTrousIterationCount");
  phiC    = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiC");
  phiN    = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiN");
  phiP    = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiP");
  minPhiZ = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.minPhiZ");
  maxPhiZ = tomlConfigReader->getConfig<float>("SvoTracerTweakingData.maxPhiZ");
  phiZStableSampleCount =
      tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiZStableSampleCount");
  changingLuminancePhi =
      tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.changingLuminancePhi");
}
