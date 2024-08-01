#pragma once

#include <memory>

struct ApplicationInfo;
struct BrushInfo;
struct CameraInfo;
struct ImguiManagerInfo;
struct ShadowMapCameraInfo;
struct SvoBuilderInfo;
struct SvoTracerInfo;
struct SvoTracerTweakingInfo;

class Logger;

struct ConfigContainer {
  ConfigContainer(Logger *logger);
  ~ConfigContainer();

  // disable move and copy
  ConfigContainer(const ConfigContainer &)            = delete;
  ConfigContainer &operator=(const ConfigContainer &) = delete;
  ConfigContainer(ConfigContainer &&)                 = delete;
  ConfigContainer &operator=(ConfigContainer &&)      = delete;

  std::unique_ptr<ApplicationInfo> applicationInfo;
  std::unique_ptr<BrushInfo> brushInfo;
  std::unique_ptr<CameraInfo> cameraInfo;
  std::unique_ptr<ImguiManagerInfo> imguiManagerInfo;
  std::unique_ptr<ShadowMapCameraInfo> shadowMapCameraInfo;
  std::unique_ptr<SvoBuilderInfo> svoBuilderInfo;
  std::unique_ptr<SvoTracerInfo> svoTracerInfo;
  std::unique_ptr<SvoTracerTweakingInfo> svoTracerTweakingInfo;

private:
  Logger *_logger;

  void _loadConfig();
};
