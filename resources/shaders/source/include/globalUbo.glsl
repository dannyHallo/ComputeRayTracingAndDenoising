layout(binding = 0) uniform GlobalUniformBufferObject {
  uint swapchainWidth;
  uint swapchainHeight;
  float vfov;
  uint currentSample;
  float time;
}
globalUbo;