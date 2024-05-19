#include <filesystem>
#include <iostream>

#include <glog/logging.h>
#include <ros/ros.h>
#include <visualization_msgs/Marker.h>
#include <wavemap/common.h>
#include <wavemap/data_structure/volumetric/hashed_wavelet_octree.h>
#include <wavemap/data_structure/volumetric/volumetric_data_structure_base.h>
#include <wavemap/utils/esdf/collision_utils.h>
#include <wavemap/utils/esdf/esdf_generator.h>
#include <wavemap/utils/random_number_generator.h>
#include <wavemap_io/file_conversions.h>
#include <wavemap_msgs/Map.h>
#include <wavemap_ros_conversions/map_msg_conversions.h>

using namespace wavemap;  // NOLINT
int main(int argc, char** argv) {
  // Initialize GLOG
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InstallFailureSignalHandler();
  FLAGS_alsologtostderr = true;
  FLAGS_colorlogtostderr = true;

  // Initialize ROS and advertise publishers
  ros::init(argc, argv, "esdf_generator");
  ros::NodeHandle nh("~");
  ros::Publisher occupancy_pub =
      nh.advertise<wavemap_msgs::Map>("map", 10, true);
  ros::Publisher esdf_pub = nh.advertise<wavemap_msgs::Map>("esdf", 10, true);
  ros::Publisher collision_free_position_pub =
      nh.advertise<visualization_msgs::Marker>("collision_free_position", 10,
                                               true);

  // Load the occupancy map
  const std::filesystem::path map_file_path =
      "/home/victor/data/wavemaps/leoc6.wvmp";
  VolumetricDataStructureBase::Ptr occupancy_map;
  io::fileToMap("/home/victor/data/wavemaps/leoc6.wvmp", occupancy_map);
  CHECK_NOTNULL(occupancy_map);

  // Publish the occupancy map
  wavemap_msgs::Map occupancy_map_msg;
  convert::mapToRosMsg(*occupancy_map, "odom", ros::Time::now(),
                       occupancy_map_msg);
  occupancy_pub.publish(occupancy_map_msg);
  ros::spinOnce();

  // Currently, only hashed wavelet octree maps are supported as input
  if (const auto hashed_map =
          std::dynamic_pointer_cast<HashedWaveletOctree>(occupancy_map);
      hashed_map) {
    HashedBlocks::Ptr esdf;
    const std::filesystem::path esdf_file_path =
        std::filesystem::path(map_file_path).replace_extension("dwvmp");
    if (std::filesystem::exists(esdf_file_path)) {
      // Load the ESDF
      LOG(INFO) << "Loading ESDF from path: " << esdf_file_path;
      VolumetricDataStructureBase::Ptr esdf_tmp;
      if (!io::fileToMap(esdf_file_path, esdf_tmp)) {
        LOG(ERROR) << "Could not load ESDF";
        return EXIT_FAILURE;
      }
      esdf = std::dynamic_pointer_cast<HashedBlocks>(esdf_tmp);
      if (!esdf) {
        LOG(ERROR) << "Loaded ESDF is not of type hashed blocks";
        return EXIT_FAILURE;
      }
    } else {
      // Generate the ESDF
      LOG(INFO) << "Generating ESDF";
      constexpr FloatingPoint kOccupancyThreshold = 0.f;
      constexpr FloatingPoint kMaxDistance = 2.f;
      esdf = std::make_shared<HashedBlocks>(
          generateEsdf(*hashed_map, kOccupancyThreshold, kMaxDistance));

      // Save the ESDF
      LOG(INFO) << "Saving ESDF to path: " << esdf_file_path;
      if (!io::mapToFile(*esdf, esdf_file_path)) {
        LOG(ERROR) << "Could not save ESDF";
        return EXIT_FAILURE;
      }
    }

    // Publish the ESDF
    LOG(INFO) << "Publishing ESDF";
    wavemap_msgs::Map msg;
    convert::mapToRosMsg(*esdf, "odom", ros::Time::now(), msg);
    esdf_pub.publish(msg);

    // Sample collision free positions
    for (int sample_idx = 0; sample_idx < 1000; ++sample_idx) {
      // Stop if ctrl+c is pressed
      if (!ros::ok()) {
        break;
      }

      // Sample a collision free position
      constexpr FloatingPoint kRobotRadius = 1.f;
      const auto collision_free_position =
          getCollisionFreePosition(*occupancy_map, *esdf, kRobotRadius);
      if (!collision_free_position) {
        LOG(ERROR) << "Getting collision free position failed. Stopping.";
        break;
      }

      // Publish the position
      visualization_msgs::Marker marker;
      marker.pose.orientation.w = 1.0;
      marker.pose.position.x = collision_free_position->x();
      marker.pose.position.y = collision_free_position->y();
      marker.pose.position.z = collision_free_position->z();
      marker.id = 100;
      marker.ns = "collision_free_pos_" + std::to_string(sample_idx);
      marker.header.frame_id = "odom";
      marker.type = visualization_msgs::Marker::SPHERE;
      marker.action = visualization_msgs::Marker::ADD;
      marker.scale.x = kRobotRadius;
      marker.scale.y = kRobotRadius;
      marker.scale.z = kRobotRadius;
      marker.color.r = 1.0;
      marker.color.a = 1.0;
      collision_free_position_pub.publish(marker);
    }

    // Keep the node alive until ctrl+c is pressed
    ros::spin();
  } else {
    LOG(ERROR)
        << "Only hashed wavelet octree occupancy maps are currently supported.";
  }
}