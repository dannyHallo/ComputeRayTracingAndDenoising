#pragma once

inline constexpr size_t kSobalBufferSize    = 256 * 256;
inline constexpr size_t kScramblingTileSize = 128 * 128 * 8;
inline constexpr size_t kRankingTileSize    = 128 * 128 * 8;

extern const int sobol_256spp_256d[kSobalBufferSize];
extern const int scramblingTile[kScramblingTileSize];
extern const int rankingTile[kRankingTileSize];