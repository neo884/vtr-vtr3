/**
 * \file image_triangulation_module.hpp
 * \brief ImageTriangulationModule class definition
 *
 * \author Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include <vtr_tactic/modules/base_module.hpp>
#include <vtr_vision/cache.hpp>

namespace vtr {
namespace vision {

/**
 * \brief A module that generates landmarks from image features. The landmark
 * point is 3D for stereo camera.
 * \details
 * requires: qdata.[rig_features, rig_calibrations]
 * outputs: qdata.[candidate_landmarks]
 *
 * This module converts stereo-matched features into landmarks with 3D points in
 * the first camera's frame. The landmarks are candidate as it has not been
 * matched to previous experiences.
 */
class ImageTriangulationModule : public tactic::BaseModule {
 public:
  /** \brief Static module identifier. \todo change this to static_name */
  static constexpr auto static_name = "image_triangulation";

  /** \brief Config parameters */
  struct Config {
    bool visualize_features;
    bool visualize_stereo_features;
    float min_triangulation_depth;
    float max_triangulation_depth;
  };

  ImageTriangulationModule(const std::string &name = static_name)
      : tactic::BaseModule{name}, config_(std::make_shared<Config>()) {}

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override;

 private:
  /**
   * \brief Generates landmarks from image features. The landmark point is 3D
   * for stereo camera.
   */
  void runImpl(tactic::QueryCache &qdata,
               const tactic::Graph::ConstPtr &) override;

  /** \brief Visualizes features and stereo features. */
  void visualizeImpl(tactic::QueryCache &qdata,
                     const tactic::Graph::ConstPtr &) override;

  std::shared_ptr<Config> config_;
};

}  // namespace vision
}  // namespace vtr
