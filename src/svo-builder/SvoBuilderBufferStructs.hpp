#pragma once

#include <cstdint>

struct G_AtomicCounter {
  uint32_t counter;
};

struct G_FragmentListEntry {
  uint32_t coordinates;
  uint32_t properties;
};

struct G_BuildInfo {
  uint32_t allocBegin;
  uint32_t allocNum;
};

struct G_IndirectData {
  uint32_t dispatchX;
  uint32_t dispatchY;
  uint32_t dispatchZ;
};