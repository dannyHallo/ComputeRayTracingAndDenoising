#include "FpsGui.hpp"

#include "app-context/VulkanApplicationContext.hpp"

#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/ImguiManagerInfo.hpp"
#include "utils/logger/Logger.hpp"
#include "window/Window.hpp"

#include "imgui.h"
#include "implot.h"

int constexpr kHistSize = 800;

FpsGui::FpsGui(Logger *logger, ConfigContainer *configContainer, Window *window)
    : _logger(logger), _configContainer(configContainer), _window(window) {
  // create x array
  _x.resize(kHistSize);
  for (int i = 0; i < kHistSize; ++i) {
    _x[i] = static_cast<float>(i);
  }

  // create y array
  _y.resize(kHistSize, 0);
}

void FpsGui::update(VulkanApplicationContext *appContext, double filteredFps) {
  int windowWidth  = 0;
  int windowHeight = 0;
  _window->getWindowDimension(windowWidth, windowHeight);

  float constexpr kHoriRatio = 0.3F;
  float constexpr kVertRatio = 0.2F;

  float constexpr kGraphPadding = 10.F;
  float const fpsWindowWidth    = windowWidth * kHoriRatio;
  float const fpsWindowHeight   = windowHeight * kVertRatio;

  ImGui::SetNextWindowSize(ImVec2(fpsWindowWidth, fpsWindowHeight));
  ImGui::SetNextWindowPos(ImVec2(windowWidth - fpsWindowWidth, windowHeight - fpsWindowHeight));

  if (!ImGui::Begin("Fps", nullptr,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
    _logger->error("failed to create fps window!");
  }

  _updateFpsHistData(filteredFps);

  // clear y array, and refill using deque
  std::fill(_y.begin(), _y.end(), 0);
  std::copy(_fpsHistory.begin(), _fpsHistory.end(),
            _y.begin() + static_cast<long long>(kHistSize - _fpsHistory.size()));

  float constexpr kYMin   = 0;
  float constexpr kYMax   = 3000.F;
  float const kGraphSizeX = fpsWindowWidth - 2 * kGraphPadding;
  float const kGraphSizeY = fpsWindowHeight - 2 * kGraphPadding;

  ImPlot::SetNextAxisLimits(ImAxis_Y1, kYMin, kYMax);

  ImGui::SetCursorPosX(kGraphPadding);
  ImGui::SetCursorPosY(kGraphPadding);

  if (!ImPlot::BeginPlot("##FpsShadedPlot", ImVec2(kGraphSizeX, kGraphSizeY),
                         ImPlotFlags_NoInputs)) {
    _logger->error("failed to begin plot!");
  }
  ImPlot::SetupAxis(ImAxis_X1, nullptr,
                    ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoTickLabels);
  ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
  ImPlot::PushStyleColor(ImPlotCol_Fill,
                         _configContainer->imguiManagerInfo->fpsGuiColor.getImVec4()); // fill color
  ImPlot::PlotShaded("", _x.data(), _y.data(), kHistSize, 0, ImPlotShadedFlags_None);
  ImPlot::EndPlot();

  ImGui::End();
}

void FpsGui::_updateFpsHistData(double fps) {
  _fpsHistory.push_back(static_cast<float>(fps));
  if (_fpsHistory.size() > kHistSize) {
    _fpsHistory.pop_front();
  }
}