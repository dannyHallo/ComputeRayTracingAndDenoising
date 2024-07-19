#include "SvoTracerTweakingData.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

SvoTracerTweakingData::SvoTracerTweakingData(TomlConfigReader *tomlConfigReader)
    : _tomlConfigReader(tomlConfigReader) {
  _loadConfig();
}

void SvoTracerTweakingData::_loadConfig() {
  debugB1 = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.debugB1");
  debugF1 = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.debugF1");

  visualizeChunks  = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.visualizeChunks");
  visualizeOctree  = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.visualizeOctree");
  beamOptimization = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.beamOptimization");
  traceIndirectRay = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.traceIndirectRay");
  taa              = _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.taa");

  sunAngleA = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunAngleA");
  sunAngleB = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunAngleB");
  auto sunCol =
      _tomlConfigReader->getConfig<std::array<float, 3>>("SvoTracerTweakingData.sunColor");
  sunColor       = glm::vec3(sunCol.at(0), sunCol.at(1), sunCol.at(2));
  sunLuminance   = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunLuminance");
  atmosLuminance = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.atmosLuminance");
  sunSize        = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.sunSize");

  temporalAlpha = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.temporalAlpha");
  temporalPositionPhi =
      _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.temporalPositionPhi");

  aTrousIterationCount =
      _tomlConfigReader->getConfig<int>("SvoTracerTweakingData.aTrousIterationCount");
  phiC    = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiC");
  phiN    = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiN");
  phiP    = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiP");
  minPhiZ = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.minPhiZ");
  maxPhiZ = _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.maxPhiZ");
  phiZStableSampleCount =
      _tomlConfigReader->getConfig<float>("SvoTracerTweakingData.phiZStableSampleCount");
  changingLuminancePhi =
      _tomlConfigReader->getConfig<bool>("SvoTracerTweakingData.changingLuminancePhi");
}