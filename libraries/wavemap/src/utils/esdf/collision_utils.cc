#include "wavemap/utils/esdf/collision_utils.h"

namespace wavemap {
std::optional<Point3D> getCollisionFreePosition(
    const VolumetricDataStructureBase& occupancy_map, const HashedBlocks& esdf,
    FloatingPoint robot_radius, std::optional<AABB<Point3D>> aabb) {
  RandomNumberGenerator rng;

  constexpr size_t kMaxAttempts = 1000;
  for (size_t attempt_idx = 0; attempt_idx < kMaxAttempts; ++attempt_idx) {
    Index3D global_index = Index3D::Zero();
    if (aabb) {
      const Point3D point{rng.getRandomRealNumber(aabb->min[0], aabb->max[0]),
                          rng.getRandomRealNumber(aabb->min[1], aabb->max[1]),
                          rng.getRandomRealNumber(aabb->min[2], aabb->max[2])};
      global_index =
          convert::pointToNearestIndex(point, 1.f / esdf.getMinCellWidth());
    } else {
      const size_t nth_block =
          rng.getRandomInteger(0ul, esdf.getBlocks().size() - 1ul);
      auto it = esdf.getBlocks().begin();
      std::advance(it, nth_block);
      if (it == esdf.getBlocks().end()) {
        continue;
      }

      const LinearIndex linear_cell_index =
          rng.getRandomInteger(0, HashedBlocks::kCellsPerBlock - 1);

      const Index3D& block_index = it->first;
      const Index3D cell_index =
          convert::linearIndexToIndex<HashedBlocks::kCellsPerSide, 3>(
              linear_cell_index);
      global_index =
          esdf.computeIndexFromBlockIndexAndCellIndex(block_index, cell_index);
    }

    Point3D position =
        convert::indexToCenterPoint(global_index, esdf.getMinCellWidth());
    if (aabb && !aabb->containsPoint(position)) {
      continue;
    }

    const FloatingPoint occupancy_value =
        occupancy_map.getCellValue(global_index);
    const bool is_free = occupancy_value < -1e-3f;
    if (!is_free) {
      continue;
    }

    const FloatingPoint esdf_value = esdf.getCellValue(global_index);
    if (esdf_value < robot_radius) {
      continue;
    }

    return position;
  }

  LOG(WARNING) << "Could not find collision free position. Giving up after "
               << kMaxAttempts << " attempts.";
  return std::nullopt;
}

std::optional<Point2D> getCollisionFree2DPosition(
    const VolumetricDataStructureBase& occupancy_map, const HashedBlocks& esdf,
    FloatingPoint robot_radius) {
  RandomNumberGenerator rng;

  constexpr size_t kMaxAttempts = 1000;
  for (size_t attempt_idx = 0; attempt_idx < kMaxAttempts; ++attempt_idx) {
    Index3D global_index = Index3D::Zero();
    
    const size_t nth_block =
        rng.getRandomInteger(0ul, esdf.getBlocks().size() - 1ul);
    auto it = esdf.getBlocks().begin();
    std::advance(it, nth_block);
    if (it == esdf.getBlocks().end()) {
      continue;
    }

    const LinearIndex linear_cell_index =
        rng.getRandomInteger(0, HashedBlocks::kCellsPerBlock - 1);

    const Index3D& block_index = it->first;
    const Index3D cell_index =
        convert::linearIndexToIndex<HashedBlocks::kCellsPerSide, 3>(
            linear_cell_index);
    global_index =
        esdf.computeIndexFromBlockIndexAndCellIndex(block_index, cell_index);

    Point3D position =
        convert::indexToCenterPoint(global_index, esdf.getMinCellWidth());

    const FloatingPoint occupancy_value =
        occupancy_map.getCellValue(global_index);
    const bool is_free = occupancy_value < -1e-3f;
    if (!is_free) {
      continue;
    }

    const FloatingPoint esdf_value = esdf.getCellValue(global_index);
    if (esdf_value < robot_radius) {
      continue;
    }


    // hack: might not be collision free
    Point2D position_2d;
    position_2d[0] = position[0];
    position_2d[1] = position[1];

    return position_2d;
  }

  LOG(WARNING) << "Could not find collision free position. Giving up after "
               << kMaxAttempts << " attempts.";
  return std::nullopt;
}
}  // namespace wavemap
