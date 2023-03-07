#include "wavemap/data_structure/volumetric/volumetric_data_structure_factory.h"

#include "wavemap/data_structure/volumetric/hashed_blocks.h"
#include "wavemap/data_structure/volumetric/hashed_wavelet_octree.h"
#include "wavemap/data_structure/volumetric/volumetric_octree.h"
#include "wavemap/data_structure/volumetric/wavelet_octree.h"

namespace wavemap {
VolumetricDataStructureBase::Ptr VolumetricDataStructureFactory::create(
    const param::Map& params,
    std::optional<VolumetricDataStructureType> default_data_structure_type) {
  std::string error_msg;
  auto type = VolumetricDataStructureType::fromParamMap(params, error_msg);
  if (type.isValid()) {
    return create(type, params);
  }

  if (default_data_structure_type.has_value()) {
    type = default_data_structure_type.value();
    LOG(WARNING) << error_msg << " Default type \"" << type.toStr()
                 << "\" will be created instead.";
    return create(default_data_structure_type.value(), params);
  }

  LOG(ERROR) << error_msg << "No default was set. Returning nullptr.";
  return nullptr;
}

VolumetricDataStructureBase::Ptr VolumetricDataStructureFactory::create(
    VolumetricDataStructureType data_structure_type, const param::Map& params) {
  const auto config = VolumetricDataStructureConfig::from(params);
  switch (data_structure_type.toTypeId()) {
    case VolumetricDataStructureType::kHashedBlocks:
      return std::make_shared<HashedBlocks>(config);
    case VolumetricDataStructureType::kOctree:
      return std::make_shared<VolumetricOctree>(config);
    case VolumetricDataStructureType::kWaveletOctree:
      return std::make_shared<WaveletOctree>(config);
    case VolumetricDataStructureType::kHashedWaveletOctree:
      return std::make_shared<HashedWaveletOctree>(config);
    default:
      LOG(ERROR) << "Attempted to create data structure with unknown type ID: "
                 << data_structure_type.toTypeId() << ". Returning nullptr.";
      return nullptr;
  }
}
}  // namespace wavemap
