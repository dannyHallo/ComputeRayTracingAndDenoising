struct material {
  uint materialType;
  vec3 albedo;
};

struct triangle {
  vec3 v0;
  vec3 v1;
  vec3 v2;
  uint materialIndex;
  uint meshHash;
};

struct light {
  uint triangleIndex;
  float area;
};

struct sphere {
  vec4 s;
  uint materialIndex;
};

struct bvhNode {
  vec3 min;
  vec3 max;
  int leftNodeIndex;
  int rightNodeIndex;
  int objectIndex;
};

struct onb {
  vec3 u;
  vec3 v;
  vec3 w;
};

const float deg2Rad = 0.0174532925;

#define LIGHT_MATERIAL 0
#define LAMBERTIAN_MATERIAL 1
#define METAL_MATERIAL 2
#define GLASS_MATERIAL 3
