// Copyright 2021, Autonomous Space Robotics Lab (ASRL)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * \file change_detection_module.cpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#include "vtr_lidar/modules/planning/change_detection_module.hpp"

#include "vtr_lidar/data_structures/costmap.hpp"

namespace vtr {
namespace lidar {

namespace {

struct AvgOp {
  using InputIt = std::vector<float>::const_iterator;

  AvgOp(const int &min_count = 1, const float &clipped_error = 0.0,
        const float &divider = 1.0)
      : min_count_(min_count),
        clipped_error_(clipped_error),
        divider_(divider) {}

  float operator()(InputIt first, InputIt last) const {
    int count = 0.;
    float sum = 0.;
    for (; first != last; ++first) {
      sum += *first;
      ++count;
    }

    if (count < min_count_ || (sum / (float)count) < clipped_error_) return 0.0;

    return (sum / (float)count) / divider_;
  }

 private:
  const int min_count_;
  const float clipped_error_;
  const float divider_;
};

}  // namespace

using namespace tactic;

auto ChangeDetectionModule::Config::fromROS(const rclcpp::Node::SharedPtr &node,
                                            const std::string &param_prefix)
    -> ConstPtr {
  auto config = std::make_shared<Config>();
  // clang-format off
  config->resolution = node->declare_parameter<float>(param_prefix + ".resolution", config->resolution);
  config->size_x = node->declare_parameter<float>(param_prefix + ".size_x", config->size_x);
  config->size_y = node->declare_parameter<float>(param_prefix + ".size_y", config->size_y);

  config->run_async = node->declare_parameter<bool>(param_prefix + ".run_async", config->run_async);
  config->visualize = node->declare_parameter<bool>(param_prefix + ".visualize", config->visualize);
  // clang-format on
  return config;
}

void ChangeDetectionModule::run_(QueryCache &qdata0, OutputCache &output0,
                                    const Graph::Ptr &graph,
                                    const TaskExecutor::Ptr &executor) {
  auto &qdata = dynamic_cast<LidarQueryCache &>(qdata0);
  // auto &output = dynamic_cast<LidarOutputCache &>(output0);

  const auto &map_id = *qdata.map_id;

  if (config_->run_async)
    executor->dispatch(std::make_shared<Task>(
        shared_from_this(), qdata0.shared_from_this(), 0, Task::DepIdSet{},
        Task::DepId{}, "Change Detection", map_id));
  else
    runAsync_(qdata0, output0, graph, executor, Task::Priority(-1),
                 Task::DepId());
}

void ChangeDetectionModule::runAsync_(QueryCache &qdata0, OutputCache &output0,
                                      const Graph::Ptr &,
                                      const TaskExecutor::Ptr &,
                                      const Task::Priority &,
                                      const Task::DepId &) {
  auto &qdata = dynamic_cast<LidarQueryCache &>(qdata0);
  auto &output = dynamic_cast<LidarOutputCache &>(output0);

  if (output.chain.valid() && qdata.map_sid.valid() &&
      output.chain->trunkSequenceId() != *qdata.map_sid) {
    CLOG(INFO, "lidar.change_detection")
        << "Trunk id has changed, skip change detection for this scan";
    return;
  }

  // visualization setup
  if (config_->visualize) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!publisher_initialized_) {
      // clang-format off
      tf_bc_ = std::make_shared<tf2_ros::TransformBroadcaster>(*qdata.node);
      scan_pub_ = qdata.node->create_publisher<PointCloudMsg>("change_detection_scan", 5);
      map_pub_ = qdata.node->create_publisher<PointCloudMsg>("change_detection_map", 5);
      ogm_pub_ = qdata.node->create_publisher<OccupancyGridMsg>("change_detection_ogm", 5);
      // clang-format on
      publisher_initialized_ = true;
    }
  }

  // inputs
  const auto &stamp = *qdata.stamp;
  const auto &T_s_r = *qdata.T_s_r;
  const auto &loc_vid = *qdata.map_id;
  const auto &loc_sid = *qdata.map_sid;
  const auto &T_r_lv = *qdata.T_r_m_loc;
  const auto &query_points = *qdata.undistorted_point_cloud;
  const auto &point_map = *qdata.curr_map_loc;
  const auto &point_map_data = point_map.point_map();
  const auto &T_lv_pm = point_map.T_vertex_map();

  CLOG(INFO, "lidar.change_detection")
      << "Change detection for lidar scan at stamp: " << stamp;

  // clang-format off
  // Eigen matrix of original data (only shallow copy of ref clouds)
  const auto query_mat = query_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::cartesian_offset());
  const auto query_norms_mat = query_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::normal_offset());

  // retrieve the pre-processed scan and convert it to the localization frame
  pcl::PointCloud<PointWithInfo> aligned_points(query_points);
  auto aligned_mat = aligned_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::cartesian_offset());
  auto aligned_norms_mat = aligned_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::normal_offset());
  // convert points into localization point map frame
  const auto T_pm_s = (T_s_r * T_r_lv * T_lv_pm).inverse().matrix();
  Eigen::Matrix3f C_pm_s = (T_pm_s.block<3, 3>(0, 0)).cast<float>();
  Eigen::Vector3f r_s_pm_in_pm = (T_pm_s.block<3, 1>(0, 3)).cast<float>();
  aligned_mat = (C_pm_s * query_mat).colwise() + r_s_pm_in_pm;
  aligned_norms_mat = C_pm_s * query_norms_mat;
  // clang-format on

  // create kd-tree of the map
  NanoFLANNAdapter<PointWithInfo> adapter(point_map_data);
  KDTreeSearchParams search_params;
  KDTreeParams tree_params(10 /* max leaf */);
  auto kdtree =
      std::make_unique<KDTree<PointWithInfo>>(3, adapter, tree_params);
  kdtree->buildIndex();

  // construct kd-tree of the localization map and compute nearest neighbors
  std::vector<long unsigned> nn_inds(aligned_points.size());
  std::vector<float> nn_dists(aligned_points.size());
  for (size_t i = 0; i < aligned_points.size(); i++) {
    KDTreeResultSet result_set(1);
    result_set.init(&nn_inds[i], &nn_dists[i]);
    kdtree->findNeighbors(result_set, aligned_points[i].data, search_params);
  }

  // compute planar distance
  for (size_t i = 0; i < aligned_points.size(); i++) {
    auto diff = aligned_points[i].getVector3fMap() -
                point_map_data[nn_inds[i]].getVector3fMap();
    // use planar distance
    nn_dists[i] =
        abs(diff.dot(point_map_data[nn_inds[i]].getNormalVector3fMap()));
  }

  // clang-format off
  // retrieve the pre-processed scan and convert it to the localization frame
  pcl::PointCloud<PointWithInfo> aligned_points2(aligned_points);
  auto aligned_mat2 = aligned_points2.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::cartesian_offset());
  auto aligned_norms_mat2 = aligned_points2.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::normal_offset());
  // convert back to the robot frame
  const auto T_r_pm = (T_pm_s * T_s_r).inverse().matrix();
  Eigen::Matrix3f C_r_pm = (T_r_pm.block<3, 3>(0, 0)).cast<float>();
  Eigen::Vector3f r_pm_r_in_r = (T_r_pm.block<3, 1>(0, 3)).cast<float>();
  aligned_mat2 = (C_r_pm * aligned_mat).colwise() + r_pm_r_in_r;
  aligned_norms_mat2 = C_r_pm * aligned_norms_mat;
  // clang-format on

  // project to 2d and construct the grid map
  const auto ogm = std::make_shared<SparseCostMap>(
      config_->resolution, config_->size_x, config_->size_y);
  /// \todo make configurable AvgOp
  ogm->update(aligned_points2, nn_dists, AvgOp(10, 0.3));
  ogm->T_vertex_this() = T_r_lv.inverse();
  ogm->vertex_id() = loc_vid;
  ogm->vertex_sid() = loc_sid;

  /// publish the transformed pointcloud
  if (config_->visualize) {
    std::unique_lock<std::mutex> lock(mutex_);

    const auto T_w_lv = [&]() {
      if (output0.chain.valid() && qdata.map_sid.valid())
        return output0.chain->pose(*qdata.map_sid);

      // publish the old map
      {
        PointCloudMsg pc2_msg;
        pcl::toROSMsg(point_map_data, pc2_msg);
        pc2_msg.header.frame_id = "world (offset)";
        pc2_msg.header.stamp = rclcpp::Time(*qdata.stamp);
        map_pub_->publish(pc2_msg);
      }

      // publish the aligned points
      {
        PointCloudMsg pc2_msg;
        pcl::toROSMsg(aligned_points, pc2_msg);
        pc2_msg.header.frame_id = "world";
        pc2_msg.header.stamp = rclcpp::Time(*qdata.stamp);
        scan_pub_->publish(pc2_msg);
      }

      // offline mode, world frame is the map frame
      return T_lv_pm.inverse();
    }();

    // publish the occupancy grid origin
    {
      Eigen::Affine3d T(T_w_lv.matrix());
      auto msg = tf2::eigenToTransform(T);
      msg.header.frame_id = "world (offset)";
      msg.header.stamp = rclcpp::Time(*qdata.stamp);
      msg.child_frame_id = "change detection";
      tf_bc_->sendTransform(msg);
    }

    // publish the occupancy grid
    {
      auto grid_msg = ogm->toStorable();
      grid_msg.header.frame_id = "change detection";
      grid_msg.header.stamp = rclcpp::Time(*qdata.stamp);
      ogm_pub_->publish(grid_msg);
    }
  }

  /// output
  auto change_detection_ogm_ref = output.change_detection_ogm.locked();
  auto &change_detection_ogm = change_detection_ogm_ref.get();
  change_detection_ogm = ogm;

  CLOG(INFO, "lidar.change_detection")
      << "Change detection for lidar scan at stamp: " << stamp << " - DONE";
}

}  // namespace lidar
}  // namespace vtr