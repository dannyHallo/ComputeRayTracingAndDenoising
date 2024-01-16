#include "FpsGui.hpp"

#include "imgui.h"

#include <vector>

int constexpr kFpsHistorySize = 1000;

FpsGui::FpsGui() = default;

void FpsGui::setActive(bool active) { _isActive = active; }

void FpsGui::update(float fps) {
  _updateFpsHistData(fps);

  // ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar;
  // ImGui::Begin("FPS", &_isActive, windowFlags);

  // plot the curve using imgui
  if (!_fpsHistory.empty()) {
    // create a temporary vector and copy the data from the deque
    // numberOfElements, defaultVal
    std::vector<float> tempFpsHistory(kFpsHistorySize, 0);
    std::copy(_fpsHistory.begin(), _fpsHistory.end(),
              tempFpsHistory.begin() +
                  static_cast<long long>(kFpsHistorySize - _fpsHistory.size()));

    float constexpr kScaleMin   = 0;
    float constexpr kScaleMax   = 1000.F;
    float constexpr kGraphSizeX = 0.F; // auto fit
    float constexpr kGraphSizeY = 80.F;

    ImGui::BeginTooltip();
    ImGui::EndTooltip();
    ImGui::PlotLines("##FPSPlotLines", tempFpsHistory.data(),
                     static_cast<int>(tempFpsHistory.size()), 0, nullptr, kScaleMin, kScaleMax,
                     ImVec2(kGraphSizeX, kGraphSizeY));
    ImGui::BeginTooltip();
  }

  // ImGui::End();
}

void FpsGui::_updateFpsHistData(float fps) {
  _fpsHistory.push_back(fps);
  if (_fpsHistory.size() > kFpsHistorySize) {
    _fpsHistory.pop_front();
  }
}