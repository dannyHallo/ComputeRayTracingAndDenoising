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