#pragma once

// TODO: use unordered_map
struct ImCoor3D {
  int x;
  int y;
  int z;

  bool operator==(const ImCoor3D &rhs) const {
    return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
  }
  // Define the operator< for Coor3D
  bool operator<(const ImCoor3D &other) const {
    // First compare by x, then by y, then by z
    if (x != other.x) {
      return x < other.x;
    }
    if (y != other.y) {
      return y < other.y;
    }
    return z < other.z;
  }

  ImCoor3D operator/(int s) const { return {x / s, y / s, z / s}; }
  ImCoor3D operator*(int s) const { return {x * s, y * s, z * s}; }
  ImCoor3D operator+(const ImCoor3D &rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
  ImCoor3D operator-(const ImCoor3D &rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
  ImCoor3D operator%(const ImCoor3D &rhs) const { return {x % rhs.x, y % rhs.y, z % rhs.z}; }
};