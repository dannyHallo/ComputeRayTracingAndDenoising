#ifndef SVO_MARCHING_GLSL
#define SVO_MARCHING_GLSL

#include "../include/core/definitions.glsl"

#include "../include/blockColor.glsl"
#include "../include/blockType.glsl"

vec3 decompressNormal(uint packed) {
  // extract the components
  uvec3 quantized;
  quantized.r = packed & 0x7F;
  quantized.g = (packed >> 7) & 0x7F;
  quantized.b = (packed >> 14) & 0x7F;

  // convert back to [-1, 1] range
  vec3 normal = vec3(quantized) / 127.0 * 2.0 - 1.0;

  return normal;
}

const uint STACK_SIZE = 23;
struct StackItem {
  uint node;
  float t_max;
} stack[STACK_SIZE + 1];

// this algorithm is from here:
// https://research.nvidia.com/sites/default/files/pubs/2010-02_Efficient-Sparse-Voxel/laine2010tr1_paper.pdf

// code reference:
// https://code.google.com/archive/p/efficient-sparse-voxel-octrees/
// https://github.com/AdamYuan/SparseVoxelOctree

// design decisions:
// 1. the position range of the octree is [1, 2], because the POP function need that bit
// comparison from the floating points ranged from [0, 1]
// 2. the traversing reduces branching by mirroring the coordinate system
// 3. all eight childrens are stored if at least one is active, so the parent node masks only need
// two bits (isLeaf and hasChild), this is different from the paper, which needs 16 bits for
// that

bool svoMarching(out float oT, out uint oIter, out vec3 oColor, out vec3 oPosition,
                 out vec3 oNextTracingPosition, out vec3 oNormal, out uint oVoxHash,
                 out bool oLightSourceHit, vec3 o, vec3 d, uint chunkBufferOffset) {
  uint parent  = 0;
  uint iter    = 0;
  uint voxHash = 0;

  vec3 t_coef = 1 / -abs(d);
  vec3 t_bias = t_coef * o;

  uint oct_mask = 0u;
  if (d.x > 0) oct_mask ^= 1u, t_bias.x = 3 * t_coef.x - t_bias.x;
  if (d.y > 0) oct_mask ^= 2u, t_bias.y = 3 * t_coef.y - t_bias.y;
  if (d.z > 0) oct_mask ^= 4u, t_bias.z = 3 * t_coef.z - t_bias.z;

  // initialize the active span of t-values
  float t_min = max(max(2 * t_coef.x - t_bias.x, 2 * t_coef.y - t_bias.y), 2 * t_coef.z - t_bias.z);
  float t_max = min(min(t_coef.x - t_bias.x, t_coef.y - t_bias.y), t_coef.z - t_bias.z);
  t_min       = max(t_min, 0);
  float h     = t_max;

  uint cur = 0;
  vec3 pos = vec3(1);
  uint idx = 0;
  if (1.5f * t_coef.x - t_bias.x > t_min) {
    idx ^= 1u, pos.x = 1.5f;
  }
  if (1.5f * t_coef.y - t_bias.y > t_min) {
    idx ^= 2u, pos.y = 1.5f;
  }
  if (1.5f * t_coef.z - t_bias.z > t_min) {
    idx ^= 4u, pos.z = 1.5f;
  }

  uint scale       = STACK_SIZE - 1;
  float scale_exp2 = 0.5;

  while (scale < STACK_SIZE) {
    ++iter;

    // parent pointer is the address of first largest sub-octree (8 in total) of the parent
    voxHash = parent + (idx ^ oct_mask);
    if (cur == 0u) cur = octreeBuffer.data[voxHash + chunkBufferOffset];

    vec3 t_corner = pos * t_coef - t_bias;
    float tc_max  = min(min(t_corner.x, t_corner.y), t_corner.z);

    if ((cur & 0x80000000u) != 0 && t_min <= t_max) {
      // INTERSECT
      float tv_max          = min(t_max, tc_max);
      float half_scale_exp2 = scale_exp2 * 0.5;
      vec3 t_center         = half_scale_exp2 * t_coef + t_corner;

      if (t_min <= tv_max) {
        // leaf node
        if ((cur & 0x40000000u) != 0) break;

        // PUSH
        if (tc_max < h) {
          stack[scale].node  = parent;
          stack[scale].t_max = t_max;
        }
        h = tc_max;

        parent = cur & 0x3FFFFFFFu;

        idx = 0u;
        --scale;
        scale_exp2 = half_scale_exp2;
        if (t_center.x > t_min) idx ^= 1u, pos.x += scale_exp2;
        if (t_center.y > t_min) idx ^= 2u, pos.y += scale_exp2;
        if (t_center.z > t_min) idx ^= 4u, pos.z += scale_exp2;

        cur   = 0;
        t_max = tv_max;

        continue;
      }
    }

    // ADVANCE
    uint step_mask = 0u;
    if (t_corner.x <= tc_max) step_mask ^= 1u, pos.x -= scale_exp2;
    if (t_corner.y <= tc_max) step_mask ^= 2u, pos.y -= scale_exp2;
    if (t_corner.z <= tc_max) step_mask ^= 4u, pos.z -= scale_exp2;

    // update active t-span and flip bits of the child slot index
    t_min = tc_max;
    idx ^= step_mask;

    // proceed with pop if the bit flips disagree with the ray direction
    if ((idx & step_mask) != 0) {
      // POP
      // find the highest differing bit between the two positions
      uint differing_bits = 0;
      if ((step_mask & 1u) != 0)
        differing_bits |= floatBitsToUint(pos.x) ^ floatBitsToUint(pos.x + scale_exp2);
      if ((step_mask & 2u) != 0)
        differing_bits |= floatBitsToUint(pos.y) ^ floatBitsToUint(pos.y + scale_exp2);
      if ((step_mask & 4u) != 0)
        differing_bits |= floatBitsToUint(pos.z) ^ floatBitsToUint(pos.z + scale_exp2);
      scale      = findMSB(differing_bits);
      scale_exp2 = uintBitsToFloat((scale - STACK_SIZE + 127u) << 23u); // exp2f(scale - s_max)

      // restore parent voxel from the stack
      parent = stack[scale].node;
      t_max  = stack[scale].t_max;

      // round cube position and extract child slot index
      uint shx = floatBitsToUint(pos.x) >> scale;
      uint shy = floatBitsToUint(pos.y) >> scale;
      uint shz = floatBitsToUint(pos.z) >> scale;
      pos.x    = uintBitsToFloat(shx << scale);
      pos.y    = uintBitsToFloat(shy << scale);
      pos.z    = uintBitsToFloat(shz << scale);
      idx      = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);

      // prevent same parent from being stored again and invalidate cached child descriptor
      h = 0, cur = 0;
    }
  }

  vec3 t_corner = t_coef * (pos + scale_exp2) - t_bias;

  vec3 norm = (t_corner.x > t_corner.y && t_corner.x > t_corner.z)
                  ? vec3(-1, 0, 0)
                  : (t_corner.y > t_corner.z ? vec3(0, -1, 0) : vec3(0, 0, -1));
  if ((oct_mask & 1u) == 0u) norm.x = -norm.x;
  if ((oct_mask & 2u) == 0u) norm.y = -norm.y;
  if ((oct_mask & 4u) == 0u) norm.z = -norm.z;

  // undo mirroring of the coordinate system
  if ((oct_mask & 1u) != 0u) pos.x = 3 - scale_exp2 - pos.x;
  if ((oct_mask & 2u) != 0u) pos.y = 3 - scale_exp2 - pos.y;
  if ((oct_mask & 4u) != 0u) pos.z = 3 - scale_exp2 - pos.z;

  // output results
  oPosition = clamp(o + t_min * d, pos, pos + scale_exp2);
  if (norm.x != 0) oPosition.x = norm.x > 0 ? pos.x + scale_exp2 + kEpsilon : pos.x - kEpsilon;
  if (norm.y != 0) oPosition.y = norm.y > 0 ? pos.y + scale_exp2 + kEpsilon : pos.y - kEpsilon;
  if (norm.z != 0) oPosition.z = norm.z > 0 ? pos.z + scale_exp2 + kEpsilon : pos.z - kEpsilon;
  // oNormal = norm;

  // scale_exp2 is the length of the edges of the voxel
  oNormal = decompressNormal((cur & 0x1FFFFF00u) >> 8);

  oNextTracingPosition = pos + scale_exp2 * 0.5 + 0.87 * scale_exp2 * oNormal;
  // oNextTracingPosition = oPosition + 1e-7 * norm;

  oLightSourceHit = false;

  uint blockType = cur & 0xFF;

  oColor = getBlockColor(blockType);
  // oColor = oNormal * 0.5 + 0.5;

  oIter    = iter;
  oVoxHash = voxHash;
  oT       = t_min;

  return scale < STACK_SIZE && t_min <= t_max;
}

#endif // SVO_MARCHING_GLSL