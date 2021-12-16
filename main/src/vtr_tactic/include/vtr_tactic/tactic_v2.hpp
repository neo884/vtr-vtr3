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
 * \file tactic.hpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include "rclcpp/rclcpp.hpp"

/// for visualization in ROS
#include <tf2/convert.h>
#include <tf2_eigen/tf2_eigen.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2_ros/transform_broadcaster.h>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>

#include "vtr_tactic/cache.hpp"
#include "vtr_tactic/pipeline_interface.hpp"
#include "vtr_tactic/pipelines/base_pipeline.hpp"
#include "vtr_tactic/tactic_interface.hpp"
#include "vtr_tactic/task_queue.hpp"
#include "vtr_tactic/types.hpp"

using OdometryMsg = nav_msgs::msg::Odometry;
using ROSPathMsg = nav_msgs::msg::Path;
using PoseStampedMsg = geometry_msgs::msg::PoseStamped;

namespace vtr {
namespace tactic {

class TacticCallbackInterface;

class TacticV2 : public PipelineInterface, public TacticInterface {
 public:
  PTR_TYPEDEFS(TacticV2);
  using Callback = TacticCallbackInterface;

  struct Config {
    using UniquePtr = std::unique_ptr<Config>;

    /** \brief Configuration for the localization chain */
    LocalizationChain::Config chain_config;

    /** \brief Number of threads for the async task queue */
    int task_queue_num_threads = 1;
    /** \brief Maximum number of queued tasks in task queue */
    int task_queue_size = -1;

    bool enable_parallelization = false;
    bool preprocessing_skippable = false;
    bool odometry_mapping_skippable = false;
    bool localization_skippable = true;

    /** \brief Whether to perform localization only on keyframe data */
    bool localization_only_keyframe = false;
    /** \brief Default localization covariance when chain is not localized. */
    Eigen::Matrix<double, 6, 6> default_loc_cov =
        Eigen::Matrix<double, 6, 6>::Zero();

    /** \brief Whether to extrapolate using STEAM trajectory for path tracker */
    bool extrapolate_odometry = false;

    /** \brief Threshold for merging <x, y, theta> */
    std::vector<double> merge_threshold{0.5, 0.25, 0.2};

    /** \brief Visualize odometry and localization via Rviz. */
    bool visualize = false;
    Eigen::Matrix<double, 3, 1> vis_loc_path_offset =
        Eigen::Matrix<double, 3, 1>::Zero();

    static UniquePtr fromROS(const rclcpp::Node::SharedPtr& node,
                             const std::string& prefix = "tactic");
  };

  TacticV2(
      Config::UniquePtr config, const BasePipeline::Ptr& pipeline,
      const OutputCache::Ptr& output, const Graph::Ptr& graph,
      const std::shared_ptr<Callback>& callback = std::make_shared<Callback>());

  ~TacticV2() { join(); }

 public:
  // clang-format off
  PipelineLock lockPipeline() override { return PipelineInterface::lockPipeline(); }
  /// \note following functions must be guaranteed to call with pipeline locked
  void setPipeline(const PipelineMode& pipeline_mode) override;
  void addRun(const bool ephemeral = false) override;
  void setPath(const VertexId::Vector& path, const bool follow = false) override;
  /// \todo
  void setTrunk(const VertexId& v) override {}
  double distanceToSeqId(const uint64_t& idx) override { return 0.0; }
  bool pathFollowingDone() override { return false; }
  bool canCloseLoop() const override { return false; }
  void connectToTrunk(bool privileged = false, bool merge = false) override {}
  void relaxGraph() override {}
  void saveGraph() override {}
  const Localization& persistentLoc() const override { return Localization(); }
  // clang-format on

 private:
  /** \brief Performs the actual preprocessing task */
  bool input_(const QueryCache::Ptr& qdata) override;

  /** \brief Performs the actual preprocessing task */
  bool preprocess_(const QueryCache::Ptr& qdata) override;

  /** \brief Performs the actual odometry mapping task */
  bool runOdometryMapping_(const QueryCache::Ptr& qdata) override;
  bool teachBranchOdometryMapping(const QueryCache::Ptr& qdata);
  bool teachMergeOdometryMapping(const QueryCache::Ptr& qdata);
  bool repeatMetricLocOdometryMapping(const QueryCache::Ptr& qdata);
  bool repeatFollowOdometryMapping(const QueryCache::Ptr& qdata);

  /** \brief Performs the actual localization task */
  bool runLocalization_(const QueryCache::Ptr& qdata) override;
  bool teachBranchLocalization(const QueryCache::Ptr& qdata);
  bool teachMergeLocalization(const QueryCache::Ptr& qdata);
  bool repeatMetricLocLocalization(const QueryCache::Ptr& qdata);
  bool repeatFollowLocalization(const QueryCache::Ptr& qdata);

 private:
  /// pipeline helper functions and states
  void addVertexEdge(const storage::Timestamp& stamp,
                     const EdgeTransform& T_r_m, const bool manual,
                     const EnvInfo& env_info);

  /**
   * \brief Whether this is the first frame of this run, only used by
   * preprocessing thread.
   * \note Only change this variable when pipeline is locked.
   */
  bool first_frame_ = true;

  /**
   * \brief Vertex id of the latest keyframe, only used by odometry thread
   * \note initialized to invalid, Only change this when pipeline is locked
   */
  VertexId current_vertex_id_ = VertexId((uint64_t)-1);

  /**
   * \brief used to determine what pipeline to use
   * \note Only change this when pipeline is locked
   */
  PipelineMode pipeline_mode_;

 private:
  Config::UniquePtr config_;
  const BasePipeline::Ptr pipeline_;
  const OutputCache::Ptr output_;
  const LocalizationChain::Ptr chain_;
  const Graph::Ptr graph_;

 private:
  /// robot status update related
  void updatePersistentLoc(const storage::Timestamp& t, const VertexId& v,
                           const EdgeTransform& T_r_v, bool localized,
                           bool reset_success = true);
  void updateTargetLoc(const storage::Timestamp& t, const VertexId& v,
                       const EdgeTransform& T_r_v, bool localized,
                       bool reset_success = true);

  std::shared_ptr<Callback> callback_;

  /** \brief Protects: persistent_loc_, target_loc_ */
  mutable std::mutex robot_state_mutex_;
  /** \brief Localization against the map, that persists across runs. */
  Localization persistent_loc_;
  /** \brief Localization against a target for merging. */
  Localization target_loc_;

  /// \note updates to these variables are protected by the tactic mutex.
  /** \brief Transformation from the latest keyframe to world frame */
  EdgeTransform T_w_m_odo_ = EdgeTransform(true);
  /** \brief Transformation from the localization keyframe to world frame */
  EdgeTransform T_w_m_loc_ = EdgeTransform(true);
  std::vector<PoseStampedMsg> keyframe_poses_;

  friend class TacticCallbackInterface;
  friend class TacticCallback;
};

class TacticCallbackInterface {
 public:
  using Ptr = std::shared_ptr<TacticCallbackInterface>;

  /** \brief callback on robot state updated: persistent, target */
  virtual void robotStateUpdated(const Localization&, const Localization&) {}
  virtual void publishPathUI(const TacticV2&) {}
  virtual void clearPathUI(const TacticV2&) {}

  virtual void publishOdometryRviz(const TacticV2&, const QueryCache&) {}
  virtual void publishPathRviz(const TacticV2&) {}
  virtual void publishLocalizationRviz(const TacticV2&, const QueryCache&) {}
};

}  // namespace tactic
}  // namespace vtr