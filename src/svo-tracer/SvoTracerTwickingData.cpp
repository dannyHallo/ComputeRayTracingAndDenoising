#include "SvoTracerTwickingData.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

#include <iostream>

SvoTracerTweakingData::SvoTracerTweakingData(TomlConfigReader *tomlConfigReader)
    : _tomlConfigReader(tomlConfigReader) {
  _loadConfig();
}

void SvoTracerTweakingData::_loadConfig() {
  visualizeOctree   = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.visualizeOctree");
  beamOptimization  = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.beamOptimization");
  traceSecondaryRay = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.traceSecondaryRay");
  taa               = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.taa");

  sunAngleA      = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunAngleA");
  sunAngleB      = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunAngleB");
  auto sunColorR = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunColorR");
  auto sunColorG = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunColorG");
  auto sunColorB = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunColorB");
  sunColor       = glm::vec3(sunColorR, sunColorG, sunColorB);
  sunLuminance   = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunLuminance");
  sunSize        = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunSize");

  temporalAlpha = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.temporalAlpha");
  temporalPositionPhi =
      _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.temporalPositionPhi");

  aTrousIterationCount =
      _tomlConfigReader->getConfig<int>("SvoTracerTweakingData.aTrousIterationCount");
  useVarianceGuidedFiltering =
      _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.useVarianceGuidedFiltering");
  useGradientInDepth =
      _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.useGradientInDepth");
  phiC = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiC");
  phiN = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiN");
  phiP = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiP");
  phiZ = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiZ");
  ignoreLuminanceAtFirstIteration =
      _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.ignoreLuminanceAtFirstIteration");
  changingLuminancePhi =
      _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.changingLuminancePhi");

  // check if sunCOlor is read correctly
  std::cout << "sunColor: " << sunColor.x << " " << sunColor.y << " " << sunColor.z << '\n';
}