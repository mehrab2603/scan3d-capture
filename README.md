# scan3d-capture
Projector-Camera Calibration / 3D Scanning Software

This software is modified from the 3D scanning software originally written by Daniel Moreno and Gabriel Taubin from Brown University http://mesh.brown.edu/calibration/software.html

**Added functions**

- CMake build system
- OpenCV 3+ support
- Spinnaker SDK integration to increase camera support (eg. Point Grey / FLIR cameras)
- Built-in Spinnaker-supported camera parameter control mechanism

**Requirements**

- Qt 5

	Download Qt 5 
	https://www.qt.io/download-qt-installer
	Select "Custom Install"
	Select "Archive" and Search
	Find Qt 5.15 and Install full Package. Will take a long time! > 1 hour to install
	 
- CMake 3.10.0+

	Install CMake
	https://cmake.org/download/
	Get Windows x64 Installer
	Add to path

- OpenCV 3+

	Install OpenCV 3 (e.g. OpenCV 3.4.16)
	https://opencv.org/releases/
	Use Windows Installer

- Visual Studio 2017
	Install Visual Studo 2017 64b (to pair with Qt 5.15)
	https://my.visualstudio.com/Downloads
	Install with Desktop Development with C++


- Spinnaker SDK 2.0.0.147 (optional)



**Installation**

In the root directory, create a directory named `build` and then inside this directory execute the following commands to build with Spinnaker support.

```
cmake -DWITH_SPINNAKER=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Be sure to build in Release mode

You will need to add OpenCV to your `PATH` if it is not already.
e.g.
C:\dev\opencv_3.4.16_install\opencv\build\x64\vc15\bin

**Usage**

Please refer to the original Brown University page for instructions on how to use this software. For Spinnaker cameras, it is needed to have the devices be assigned IP addressess from the same network using the Force IP feature of the FlyCapture software or using similar features from other softwares (eg. SpinView) before using with this software. Spinnaker-supported cameras will have a button appear beside their names in the `Capture` dialog to allow for adjusting camera parameters.
