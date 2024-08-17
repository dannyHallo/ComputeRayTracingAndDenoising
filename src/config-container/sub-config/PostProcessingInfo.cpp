#include "PostProcessingInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void PostProcessingInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  temporalAlpha = tomlConfigReader->getConfig<float>("PostProcessingInfo.temporalAlpha");
  temporalPositionPhi =
      tomlConfigReader->getConfig<float>("PostProcessingInfo.temporalPositionPhi");

  aTrousIterationCount =
      tomlConfigReader->getConfig<int>("PostProcessingInfo.aTrousIterationCount");
  phiC    = tomlConfigReader->getConfig<float>("PostProcessingInfo.phiC");
  phiN    = tomlConfigReader->getConfig<float>("PostProcessingInfo.phiN");
  phiP    = tomlConfigReader->getConfig<float>("PostProcessingInfo.phiP");
  minPhiZ = tomlConfigReader->getConfig<float>("PostProcessingInfo.minPhiZ");
  maxPhiZ = tomlConfigReader->getConfig<float>("PostProcessingInfo.maxPhiZ");
  phiZStableSampleCount =
      tomlConfigReader->getConfig<float>("PostProcessingInfo.phiZStableSampleCount");
  changingLuminancePhi =
      tomlConfigReader->getConfig<bool>("PostProcessingInfo.changingLuminancePhi");

  taa = tomlConfigReader->getConfig<bool>("PostProcessingInfo.taa");

  explosure = tomlConfigReader->getConfig<float>("PostProcessingInfo.explosure");
}
