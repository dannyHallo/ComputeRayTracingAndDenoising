layout(binding = 0) uniform GlobalUniformBufferObject {
  vec3 camPos;
  vec3 camFront;
  vec3 camUp;
  vec3 camRight;
  uint swapchainWidth;
  uint swapchainHeight;
  float vfov;
  uint currentSample;
  float time;
}
globalUbo;