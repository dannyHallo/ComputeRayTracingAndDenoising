#pragma once

#include "entt/signal/dispatcher.hpp" // IWYU pragma: export

namespace GlobalEventDispatcher {
extern entt::dispatcher gGlobalEventDispatcher;
inline entt::dispatcher &get() { return gGlobalEventDispatcher; }
}; // namespace GlobalEventDispatcher
