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
 * \file costmap.cpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#include "vtr_lidar/data_structures/costmap.hpp"

namespace vtr {
namespace lidar {

BaseCostMap::BaseCostMap(const float& dl, const float& size_x,
                         const float& size_y, const float& default_value)
    : dl_(dl),
      size_x_(abs(size_x)),
      size_y_(abs(size_y)),
      default_value_(default_value),
      width_(2 * std::round(size_x_ / 2.0f / dl_) + 1),
      height_(2 * std::round(size_y_ / 2.0f / dl_) + 1),
      origin_(-std::round(size_x_ / 2.0f / dl_),
              -std::round(size_y_ / 2.0f / dl_)) {
#if false
    LOG(DEBUG) << "BaseCostMap: dl: " << dl_ << ", size_x_: " << size_x_
               << ", size_y_: " << size_y_ << ", width_: " << width_
               << ", height_: " << height_ << ", origin_: " << origin_.x << ", "
               << origin_.y;
#endif
}

DenseCostMap::DenseCostMap(const float& dl, const float& size_x,
                           const float& size_y, const float& default_value)
    : BaseCostMap(dl, size_x, size_y, default_value),
      values_(Eigen::MatrixXf::Constant(width_, height_, default_value_)) {}

auto DenseCostMap::toStorable() const -> CostMapMsg {
  CostMapMsg storable;

  // transform from (0, 0) of the ros msg to (0, 0) of this class
  Eigen::Matrix4d T_this_ros_mat = Eigen::Matrix4d::Identity();
  T_this_ros_mat(0, 3) = origin_.x * dl_ - dl_ / 2.0;
  T_this_ros_mat(1, 3) = origin_.y * dl_ - dl_ / 2.0;
  tactic::EdgeTransform T_this_ros(T_this_ros_mat);
  const auto T_vertex_ros = T_vertex_this_ * T_this_ros;

  // clamp and fill in data
  std::vector<int8_t> data(width_ * height_, default_value_);
  for (size_t x = 0; x < width_; ++x)
    for (size_t y = 0; y < height_; ++y)
      data[x + y * width_] =
          (int8_t)(std::clamp(values_(x, y), 0.f, 1.f) * 100);

  storable.info.resolution = dl_;
  storable.info.width = width_;
  storable.info.height = height_;
  storable.info.origin = common::conversions::toPoseMessage(T_vertex_ros);
  storable.data = data;

  return storable;
}

void DenseCostMap::update(const std::unordered_map<PixKey, float>& values) {
  for (const auto& val : values) {
    const auto shifted_k = val.first - origin_;
    values_(shifted_k.x, shifted_k.y) = val.second;
  }
}

auto DenseCostMap::filter(const float& threshold) const -> XY2ValueMap {
  XY2ValueMap filtered;
  filtered.reserve(values_.size());
  for (int x = 0; x < width_; ++x)
    for (int y = 0; y < height_; ++y) {
      if (values_(x, y) < threshold) continue;
      const auto key = PixKey(x, y) + origin_;
      filtered.emplace(
          std::make_pair((float)(key.x * dl_), (float)(key.y * dl_)),
          values_(x, y));
    }
  return filtered;
}

SparseCostMap::SparseCostMap(const float& dl, const float& size_x,
                             const float& size_y, const float& default_value)
    : BaseCostMap(dl, size_x, size_y, default_value) {}

DenseCostMap SparseCostMap::toDense() const {
  DenseCostMap dense_cost_map(dl_, size_x_, size_y_, default_value_);
  dense_cost_map.update(values_);
  dense_cost_map.vertex_sid() = vertex_sid_;
  dense_cost_map.vertex_id() = vertex_id_;
  dense_cost_map.T_vertex_this() = T_vertex_this_;
  return dense_cost_map;
}

auto SparseCostMap::filter(const float& threshold) const -> XY2ValueMap {
  XY2ValueMap filtered;
  filtered.reserve(values_.size());
  for (const auto& val : values_) {
    const auto key = val.first;
    if (val.second < threshold) continue;
    filtered.emplace(std::make_pair((float)(key.x * dl_), (float)(key.y * dl_)),
                     val.second);
  }
  return filtered;
}

auto SparseCostMap::toStorable() const -> CostMapMsg {
  CostMapMsg storable;

  // transform from (0, 0) of the ros msg to (0, 0) of this class
  Eigen::Matrix4d T_this_ros_mat = Eigen::Matrix4d::Identity();
  T_this_ros_mat(0, 3) = origin_.x * dl_ - dl_ / 2.0;
  T_this_ros_mat(1, 3) = origin_.y * dl_ - dl_ / 2.0;
  tactic::EdgeTransform T_this_ros(T_this_ros_mat);
  const auto T_vertex_ros = T_vertex_this_ * T_this_ros;

  // clamp and fill in data
  std::vector<int8_t> data(width_ * height_, default_value_);
  for (const auto& val : values_) {
    const auto shifted_k = val.first - origin_;
    data[shifted_k.x + shifted_k.y * width_] =
        (int8_t)(std::clamp(val.second, 0.f, 1.f) * 100);
  }

  storable.info.resolution = dl_;
  storable.info.width = width_;
  storable.info.height = height_;
  storable.info.origin = common::conversions::toPoseMessage(T_vertex_ros);
  storable.data = data;

  return storable;
}

}  // namespace lidar
}  // namespace vtr