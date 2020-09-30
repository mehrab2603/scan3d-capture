# scan3d-capture
Projector-Camera Calibration / 3D Scanning Software

This software is modified from the 3D scanning software originally written by Daniel Moreno and Gabriel Taubin from Brown University http://mesh.brown.edu/calibration/software.html

**Added functions**

- Move to CMake build system
- Integrate the Spinnaker SDK to increase camera support

**Requirements**

- CMake 3.10.0+
- OpenCV 2
- Spinnaker SDK 2.0.0.147 (optional)

**Installation**

In the root directory, create a directory named `build` and then inside this directory execute the following commands (assuming that the install directory is `/home/dummy_user/scan3d` and Spinnaker support is desired).

```
cmake -DWITH_SPINNAKER=ON -DCMAKE_INSTALL_PREFIX=/home/dummy_user/scan3d ..
make
make install
```
