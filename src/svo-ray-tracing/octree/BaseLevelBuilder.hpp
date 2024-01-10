#pragma once

#include "svo-ray-tracing/im-data/ImCoor.hpp"
#include "svo-ray-tracing/im-data/ImData.hpp"

#include <memory>

namespace BaseLevelBuilder {
void build(ImageData *imageData, ImCoor3D const &imageSize);
};