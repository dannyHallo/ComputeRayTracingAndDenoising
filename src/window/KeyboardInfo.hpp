#pragma once

#include <unordered_map>

struct KeyboardInfo {
  // TODO: change this to a array to lower the cost
  std::unordered_map<int, bool> activeKeyMap;

  [[nodiscard]] bool isKeyPressed(int key) const {
    auto it = activeKeyMap.find(key);
    if (it == activeKeyMap.end()) {
      return false;
    }
    return it->second;
  }

  void disableInputBit(int bitToBeDisabled) { activeKeyMap[bitToBeDisabled] = false; }
};