# Visual Teach &amp; Repeat 3 (VT&amp;R3)

## Update (2022-02-24)
For code related to [Should Radar Replace Lidar in All-Weather Mapping and Localization](https://arxiv.org/abs/2203.10174) (Keenan Burnett, Yuchen Wu, David J. Yoon, Angela P. Schoellig, Timothy D. Barfoot, 2022), please see the `radar_lidar_dev` branch. Code related to lidar teach and repeat is located under `main/src/vtr_lidar` and code related to radar teach and repeat is located under `main/src/vtr_radar`. We are working on merging this code into the `main` branch.

## What is VT&amp;R3?

VT&amp;R3 is a C++ implementation of the Teach and Repeat navigation framework. It enables a robot to be taught a network of traversable paths and then accurately repeat any network portion. VT&amp;R3 is designed for easy adaptation to various sensor (camera/LiDAR/RaDAR/GPS) and robot combinations. The current implementation includes a feature-based visual odometry and localization pipeline that estimates the robot's motion from stereo camera images and a point-cloud-based odometry and localization pipeline for LiDAR sensors.

## Installation and Getting Started

The following articles will help you get started with VT&amp;R3:

- [Installation Guide](https://github.com/utiasASRL/vtr3/wiki/Installation-Guide)
- [Launching VT&amp;R3](https://github.com/utiasASRL/vtr3/wiki/Launching-VTR3)
- [Setting Up VT&amp;R3 with a New Sensor or Robot](https://github.com/utiasASRL/vtr3/wiki/Setting-Up-VTR3-with-a-New-Sensor-or-Robot)
- [VT&amp;R3 Sample Datasets](https://github.com/utiasASRL/vtr3/wiki/VTR3-Sample-Datasets)

More information can be found in the [wiki page](https://github.com/utiasASRL/vtr3/wiki).

## Citation

Please cite the [following paper](https://onlinelibrary.wiley.com/doi/full/10.1002/rob.20342) when using VT&amp;R3 for your research:

```bibtex
@article{paul2010vtr,
  author = {Furgale, Paul and Barfoot, Timothy D.},
  title = {Visual teach and repeat for long-range rover autonomy},
  journal = {Journal of Field Robotics},
  year = {2010},
  doi = {https://doi.org/10.1002/rob.20342}
}
```

### Additional Citations

- [Multi-Experience Localization](https://ieeexplore.ieee.org/abstract/document/7759303)
  ```bibtex
  @inproceedings{michael2016mel,
    author = {Paton, Michael and MacTavish, Kirk and Warren, Michael and Barfoot, Timothy D.},
    booktitle = {2016 IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS)},
    title = {Bridging the appearance gap: Multi-experience localization for long-term visual teach and repeat},
    year={2016},
    doi={10.1109/IROS.2016.7759303}
  }
  ```
 - [Lidar, Radar Teach and Repeat](https://arxiv.org/abs/2203.10174)
    ```bibtex
      @article{burnett_radar22,
      author = {Burnett, Keenan and Wu, Yuchen and Yoon, David J., Schoellig, Angela P. and Barfoot, Timothy D.},
      title = {Should Radar Replace Lidar in All-Weather Mapping and Localization?},
      journal={arXiv preprint arXiv:2203.10174},
      year={2022}
    }
    ```

## [License](./LICENSE)
