# scan3d-capture
Projector-Camera Calibration / 3D Scanning Software

This software is modified from the 3D scanning software originally written by Daniel Moreno and Gabriel Taubin from Brown University http://mesh.brown.edu/calibration/software.html

**Added functions**

- CMake build system
- OpenCV 3+ support
- Spinnaker SDK integration to increase camera support (eg. Point Grey / FLIR cameras)
- Built-in Spinnaker-supported camera parameter control mechanism

**Requirements**

- CMake 3.10.0+
- OpenCV 3+
- Spinnaker SDK 2.0.0.147 (optional)

**Installation**

In the root directory, create a directory named `build` and then inside this directory execute the following commands to build with Spinnaker support.

```
cmake -DWITH_SPINNAKER=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

You will need to add OpenCV to your `PATH` if it is not already.

**Usage**

Please refer to the original Brown University page for instructions on how to use this software. For Spinnaker cameras, it is needed to have the devices be assigned IP addressess from the same network using the Force IP feature of the FlyCapture software or using similar features from other softwares (eg. SpinView) before using with this software. Spinnaker-supported cameras will have a button appear beside their names in the `Capture` dialog to allow for adjusting camera parameters.
