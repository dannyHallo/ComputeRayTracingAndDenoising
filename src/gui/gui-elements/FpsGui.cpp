#include "FpsGui.hpp"

#include "imgui.h"
#include "implot.h"

#include "utils/math/MovingAvg.hpp"

int constexpr kHistSize = 800;

FpsGui::FpsGui() {
  // create x array
  _x.resize(kHistSize);
  for (int i = 0; i < kHistSize; ++i) {
    _x[i] = static_cast<float>(i);
  }

  // create y array
  _y.resize(kHistSize, 0);
}

void FpsGui::setActive(bool active) { _isActive = active; }

void FpsGui::update(double fps) {
  static MovingAvg avg(20);
  avg.add(static_cast<float>(fps));

  _updateFpsHistData(avg.getAverage());

  // clear y array, and refill using deque
  std::fill(_y.begin(), _y.end(), 0);
  std::copy(_fpsHistory.begin(), _fpsHistory.end(),
            _y.begin() + static_cast<long long>(kHistSize - _fpsHistory.size()));

  float constexpr kYMin       = 0;
  float constexpr kYMax       = 3000.F;
  float constexpr kGraphSizeX = 0.F; // auto fit
  float constexpr kGraphSizeY = 120.F;

  ImPlot::SetNextAxisLimits(ImAxis_Y1, kYMin, kYMax);

  ImPlot::BeginPlot("##FpsShadedPlot", ImVec2(kGraphSizeX, kGraphSizeY), ImPlotFlags_NoInputs);
  ImPlot::SetupAxis(ImAxis_X1, nullptr,
                    ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoTickLabels);
  ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_AutoFit);
  ImPlot::PlotShaded("", _x.data(), _y.data(), kHistSize, 0, ImPlotShadedFlags_None);
  ImPlot::EndPlot();
}

void FpsGui::_updateFpsHistData(double fps) {
  _fpsHistory.push_back(static_cast<float>(fps));
  if (_fpsHistory.size() > kHistSize) {
    _fpsHistory.pop_front();
  }
}