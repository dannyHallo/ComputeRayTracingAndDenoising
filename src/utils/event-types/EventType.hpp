#pragma once

#include <cstdint>

// modules can use this event to make a quest to the caller to block the render loop
struct E_RenderLoopBlockRequest {
  uint32_t blockStateBits;
};

// after the caller's render loop comes to a halt, this event is triggered
struct E_RenderLoopBlocked {};
