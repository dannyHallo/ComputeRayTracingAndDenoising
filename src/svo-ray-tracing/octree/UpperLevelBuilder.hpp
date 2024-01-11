#pragma once

#include "svo-ray-tracing/im-data/ImCoor.hpp"
#include "svo-ray-tracing/im-data/ImData.hpp"

#include <memory>

namespace UpperLevelBuilder {
void build(ImData const *lowerLevelData, ImData *thisLevelData, bool isBaseLevel);
};