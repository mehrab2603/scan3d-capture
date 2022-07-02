# procamcalib

Projector-Camera Calibration Application

This software was originally written by the Daniel Moreno and Gabriel Taubin
from Brown University, which can be found at this
[link](http://mesh.brown.edu/calibration/software.html).
Please see LICENSE.txt

This software was then modified by GitHub user `mehrab2603`, who added CMake
support, OpenCV 3+ support, and Spinnaker SDK integrations to allow support of
Point Grey / FLIR cameras. Their work heavily aided the Photoneo camera support
this fork adds. Their repository can be found at this
[link](https://github.com/mehrab2603/scan3d-capture).

## Features

    - CMake build system
    - OpenCV 3+ support
    - Photoneo Camera Support

## Prerequisites

1. Qt 5.15
    
    1. Download and run the [Qt 5.15 installer](https://www.qt.io/download-qt-installer)
    2. Select "Custom Install" in the installation type
    3. Check the "Archive" checkbox to find the 5.15 release
    4. Install the full Qt 5.15 installation. This will take > 1 hr to install

2. CMake 3.10.0+

	1. Download and run the [CMake installer](https://cmake.org/download/)
    2. When prompted in the installer, add to `PATH`

2. OpenCV 3.4.16

    1. Download and run the [OpenCV 3.4.16 installer](https://opencv.org/releases/)
    2. Note your install path, you will need to provide it to CMake later

3. Visual Studio 2017

    1. Download and run the [Visual Studio 2016 64-bit installer](https://my.visualstudio.com/Downloads)
    2. When prompted, install with "Desktop Development with C++}"

4. Photoneo PhoXi Control (optional)

    1. Download and run the [PhoXi Control Installer](https://www.photoneo.com/downloads/phoxi-control/)
    2. Select "PhoXi Control" in the dropdown menu and download the "Windows 10" installer
	3. Follow all of the prompts and select the "Full" install type
	4. The files should be installed to C:/Progam Files/Photoneo, which is the default option

5. Spinnaker SDK 2.0.0.147 (optional - no ongoing support for this SDK)

## Installation

To build with Photoneo support: 

1. In the top level dir, create a `build` directory

2. Open the top level dir in the CMake GUI and set `build` as the build dir

3. Select the `WITH_PHOTONEO`checkbox to enable Photoneo support

4. Click "Configure"

5. Set `OpenCV_DIR` to the appropriate location. In our installation, this was in
   `C:/dev/opencv_3.4.16_install/opencv/build`

6. Click "Configure" - at this point, no red entries should appear

7. Click "Generate" then "Open Project" to open the project in Visual Studio

8. Be sure to build in Release Mode

9. You will need to add OpenCV to your `PATH` if it is not already, e.g.
   `C:\dev\opencv_3.4.16_install\opencv\build\x64\vc15\bin`

# Usage

TODO: Copy Ye Min's guidelines here.
