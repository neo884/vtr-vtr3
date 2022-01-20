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
 * \file localization_map_recall_module.hpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include "vtr_lidar/cache.hpp"
#include "vtr_tactic/modules/base_module.hpp"
#include "vtr_tactic/task_queue.hpp"

#include <sensor_msgs/msg/point_cloud2.hpp>

namespace vtr {
namespace lidar {

class LocalizationMapRecallModule : public tactic::BaseModule {
 public:
  using PointCloudMsg = sensor_msgs::msg::PointCloud2;

  /** \brief Static module identifier. */
  static constexpr auto static_name = "lidar.localization_map_recall";

  /** \brief Config parameters */
  struct Config : public tactic::BaseModule::Config {
    using Ptr = std::shared_ptr<Config>;
    using ConstPtr = std::shared_ptr<const Config>;

    std::string map_version = "multi_exp_point_map";
    bool visualize = false;

    static ConstPtr fromROS(const rclcpp::Node::SharedPtr &node,
                            const std::string &param_prefix);
  };

  LocalizationMapRecallModule(
      const Config::ConstPtr &config,
      const std::shared_ptr<tactic::ModuleFactory> &module_factory = nullptr,
      const std::string &name = static_name)
      : tactic::BaseModule{module_factory, name}, config_(config) {}

 private:
  void run_(tactic::QueryCache &qdata, tactic::OutputCache &output,
               const tactic::Graph::Ptr &graph,
               const tactic::TaskExecutor::Ptr &executor) override;

  Config::ConstPtr config_;

  /** \brief for visualization only */
  bool publisher_initialized_ = false;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr map_pub_;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr test_map_pub_;

  VTR_REGISTER_MODULE_DEC_TYPE(LocalizationMapRecallModule);
};

}  // namespace lidar
}  // namespace vtr