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
 * \file detector.cpp
 * \author Keenan Burnett, Autonomous Space Robotics Lab (ASRL)
 * \brief Keypoint extraction methods for Navtech radar
 */
#pragma once

#include "vtr_radar/detector/detector.hpp"

namespace vtr {
namespace radar {

namespace {
bool comp(const std::pair<int, float> &a,
               const std::pair<int, float> &b) {
  return (a.first > b.first);
}
}  // namespace

template <class PointT>
void KStrongest<PointT>::run(const cv::Mat &raw_scan, const float &res,
                             const std::vector<double> &azimuth_times,
                             const std::vector<double> &azimuth_angles,
                             pcl::PointCloud<PointT> &pointcloud) {
  pointcloud.clear();
  const int rows = raw_scan.rows;
  const int cols = raw_scan.cols;
  auto mincol = minr_ / res;
  if (mincol > cols || mincol < 0) mincol = 0;
  auto maxcol = maxr_ / res;
  if (maxcol > cols || maxcol < 0) maxcol = cols;
  const auto N = maxcol - mincol;

#pragma omp parallel for
  for (int i = 0; i < rows; ++i) {
    std::vector<std::pair<float, int>> intens;
    intens.reserve(N / 2);
    double mean = 0;
    for (int j = mincol; j < maxcol; ++j) {
      mean += raw_scan.at<float>(i, j);
    }
    mean /= N;
    const double thres = mean * threshold_;
    for (int j = mincol; j < maxcol; ++j) {
      if (raw_scan.at<float>(i, j) >= thres)
        intens.emplace_back(raw_scan.at<float>(i, j), j);
    }
    // sort intensities in descending order
    std::sort(intens.begin(), intens.end(), comp);
    const double azimuth = azimuth_angles[i];
    const double time = azimuth_times[i];
    pcl::PointCloud<PointT> polar_time;
    for (int j = 0; j < kstrong_; ++j) {
      if (intens[j].first < thres) break;
      PointT p;
      p.rho = float(intens[j].second) * res;
      p.phi = azimuth;
      p.theta = 0;
      p.time = time;
      polar_time.push_back(p);
    }
#pragma omp critical
    {
      pointcloud.insert(pointcloud.end(), polar_time.begin(), polar_time.end());
    }
  } 
}

template <class PointT>
void Cen2018<PointT>::run(const cv::Mat &raw_scan, const float &res,
                          const std::vector<double> &azimuth_times,
                          const std::vector<double> &azimuth_angles,
                          pcl::PointCloud<PointT> &pointcloud) {
  pointcloud.clear();
  const int rows = raw_scan.rows;
  const int cols = raw_scan.cols;
  auto mincol = minr_ / res;
  if (mincol > cols || mincol < 0) mincol = 0;
  auto maxcol = maxr_ / res;
  if (maxcol > cols || maxcol < 0) maxcol = cols;
  const auto N = maxcol - mincol;

  std::vector<float> sigma_q(rows, 0);
  // TODO: try implementing an efficient median filter
  // Estimate the bias and subtract it from the signal
  cv::Mat q = raw_scan.clone();
  for (int i = 0; i < rows; ++i) {
    float mean = 0;
    for (int j = mincol; j < maxcol; ++j) {
      mean += raw_scan.at<float>(i, j);
    }
    mean /= N;
    for (int j = mincol; j < maxcol; ++j) {
      q.at<float>(i, j) = raw_scan.at<float>(i, j) - mean;
    }
  }

  // Create 1D Gaussian Filter
  // TODO: binomial filter may be more efficient
  int fsize = sigma_ * 2 * 3;
  if (fsize % 2 == 0)
    fsize += 1;
  const int mu = fsize / 2;
  const float sig_sqr = sigma_ * sigma_;
  cv::Mat filter = cv::Mat::zeros(1, fsize, CV_32F);
  float s = 0;
  for (int i = 0; i < fsize; ++i) {
    filter.at<float>(0, i) = exp(-0.5 * (i - mu) * (i - mu) / sig_sqr);
    s += filter.at<float>(0, i);
  }
  filter /= s;
  cv::Mat p;
  cv::filter2D(q, p, -1, filter, cv::Point(-1, -1), 0, cv::BORDER_REFLECT101);

  // Estimate variance of noise at each azimuth
  for (int i = 0; i < rows; ++i) {
    int nonzero = 0;
    for (int j = mincol; j < maxcol; ++j) {
      float n = q.at<float>(i, j);
      if (n < 0) {
        sigma_q[i] += 2 * (n * n);
        nonzero++;
      }
    }
    if (nonzero)
      sigma_q[i] = sqrt(sigma_q[i] / nonzero);
    else
      sigma_q[i] = 0.034;
  }
  // Extract peak centers from each azimuth
#pragma omp parallel for
  for (int i = 0; i < rows; ++i) {
    pcl::PointCloud<PointT> polar_time;
    float peak_points = 0;
    int num_peak_points = 0;
    const float thres = zq_ * sigma_q[i];
    const double azimuth = azimuth_angles[i];
    const double time = azimuth_times[i];
    for (int j = mincol; j < maxcol; ++j) {
      const float nqp = exp(-0.5 * pow((q.at<float>(i, j) - p.at<float>(i, j)) / sigma_q[i], 2));
      const float npp = exp(-0.5 * pow(p.at<float>(i, j) / sigma_q[i], 2));
      const float b = nqp - npp;
      const float y = q.at<float>(i, j) * (1 - nqp) + p.at<float>(i, j) * b;
      if (y > thres) {
        peak_points += j;
        num_peak_points += 1;
      } else if (num_peak_points > 0) {
        PointT p;
        p.rho = res * peak_points / num_peak_points;
        p.phi = azimuth;
        p.theta = 0;
        p.time = time;
        polar_time.push_back(p);
        peak_points = 0;
        num_peak_points = 0;
      }
    }
    if (num_peak_points > 0) {
      const double azimuth = azimuth_angles[rows - 1];
      const double time = azimuth_times[rows - 1];
      PointT p;
      p.rho = res * peak_points / num_peak_points;
      p.phi = azimuth;
      p.theta = 0;
      p.time = time;
      polar_time.push_back(p);
    }
#pragma omp critical
    {
      pointcloud.insert(pointcloud.end(), polar_time.begin(), polar_time.end());
    }
  }
}

template <class PointT>
void CACFAR<PointT>::run(const cv::Mat &raw_scan, const float &res,
                         const std::vector<double> &azimuth_times,
                         const std::vector<double> &azimuth_angles,
                         pcl::PointCloud<PointT> &pointcloud) {}  // TODO

template <class PointT>
void OSCFAR<PointT>::run(const cv::Mat &raw_scan, const float &res,
                         const std::vector<double> &azimuth_times,
                         const std::vector<double> &azimuth_angles,
                         pcl::PointCloud<PointT> &pointcloud) {}  // TODO

}  // namespace radar
}  // namespace vtr