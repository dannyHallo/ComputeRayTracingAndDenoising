#pragma once

// modules can use this event to make a quest to the caller to block the render loop
struct E_RenderLoopBlockRequest {};

// after the caller's render loop comes to a halt, this event is triggered
struct E_RenderLoopBlocked {};