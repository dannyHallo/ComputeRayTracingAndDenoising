#ifndef SVO_TRACER_DATA_H
#define SVO_TRACER_DATA_H

struct G_RenderInfo {
  vec3 camPosition;
  vec3 camFront;
  vec3 camUp;
  vec3 camRight;
  uint swapchainWidth;
  uint swapchainHeight;
  float vfov;
  uint currentSample;
  float time;
};

struct G_TwickableParameters {
  uint magicButton;
};

#endif