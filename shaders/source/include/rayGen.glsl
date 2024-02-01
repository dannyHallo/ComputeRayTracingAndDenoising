#ifndef RAY_GEN_H
#define RAY_GEN_H

// each component of coord ranges from 0 to 1
vec3 getRayDir(G_RenderInfo rInfo, vec2 uv) {
  float aspectRatio        = float(rInfo.swapchainWidth) / float(rInfo.swapchainHeight);
  float viewportHalfHeight = tan(rInfo.vfov * kDeg2Rad * 0.5);
  float viewportHalfWidth  = aspectRatio * viewportHalfHeight;

  uv = uv * 2.0 - 1.0;

  return normalize(rInfo.camFront + rInfo.camRight * viewportHalfWidth * uv.x -
                   rInfo.camUp * viewportHalfHeight * uv.y);
}

#endif // RAY_GEN_H