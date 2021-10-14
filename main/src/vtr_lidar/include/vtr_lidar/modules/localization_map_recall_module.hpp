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
 * \brief LocalizationMapRecallModule class definition
 *
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include <vtr_lidar/cache.hpp>
#include <vtr_tactic/modules/base_module.hpp>

#include <sensor_msgs/msg/point_cloud2.hpp>

namespace vtr {
namespace lidar {

class LocalizationMapRecallModule : public tactic::BaseModule {
 public:
  using PointCloudMsg = sensor_msgs::msg::PointCloud2;

  /** \brief Static module identifier. */
  static constexpr auto static_name = "lidar.localization_map_recall";

  /** \brief Config parameters */
  struct Config {
    bool visualize = false;
  };

  LocalizationMapRecallModule(const std::string &name = static_name)
      : tactic::BaseModule{name}, config_(std::make_shared<Config>()) {}

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override;

 private:
  void runImpl(tactic::QueryCache &qdata,
               const tactic::Graph::ConstPtr &graph) override;

  std::shared_ptr<Config> config_;

  /** \brief for visualization only */
  rclcpp::Publisher<PointCloudMsg>::SharedPtr map_pub_;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr test_map_pub_;
  bool publisher_initialized_ = false;
};

}  // namespace lidar
}  // namespace vtr