#include "ImguiManagerInfo.hpp"

#include "utils/toml-config/TomlConfigReader.hpp"

void ImguiManagerInfo::loadConfig(TomlConfigReader *tomlConfigReader) {
  fontSize       = tomlConfigReader->getConfig<float>("ImguiManager.fontSize");
  auto const &gc = tomlConfigReader->getConfig<std::array<int, 3>>("ImguiManager.fpsGuiColor");
  fpsGuiColor    = Color(gc.at(0), gc.at(1), gc.at(2));
  auto const &bb =
      tomlConfigReader->getConfig<std::array<int, 3>>("ImguiManager.menuBarBackgroundColor");
  menuBarBackgroundColor = Color(bb.at(0), bb.at(1), bb.at(2));
  auto const &pb =
      tomlConfigReader->getConfig<std::array<int, 3>>("ImguiManager.popupBackgroundColor");
  popupBackgroundColor = Color(pb.at(0), pb.at(1), pb.at(2));
}
