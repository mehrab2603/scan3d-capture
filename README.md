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

3. Visual Studio 2022

  1. Download and run the [Visual Studio 2022 64-bit installer](https://my.visualstudio.com/Downloads)
  2. When prompted, install with "Desktop Development with C++"

4. OpenCV 4.7.0

  1. Download the [OpenCV 4.7.0 source](https://opencv.org/releases/)
  2. Extract the OpenCV source files - we suggest into `dev/opencv-4.7.0/`
  3. Create a build folder - we suggest `dev/opencv-4.7.0/build`
  4. Create an install folder - we suggest `dev/opencv-4.7.0/install`
  5. Use the CMake GUI to generate the Visual Studio solution. Note that you'll want to set the install folder path appropriately.
  6. If needed, modify the OpenCV Files to support off-image principal points for projectors with lens offsets.
     Navigate to `calibration.cpp` and comment out the following lines (lines 1548 - 1550)
     ```
     if( A(0, 2) < 0 || A(0, 2) >= imageSize.width ||
         A(1, 2) < 0 || A(1, 2) >= imageSize.height )
         CV_Error( CV_StsOutOfRange, "Principal point must be within the image" );
      
     ```
  7. Run the `ALL_BUILD` and `INSTALL` CMake targets
  8. Note your install path, you will need to provide it to CMake later

4. Photoneo PhoXi Control (optional)

  1. Download and run the [PhoXi Control Installer](https://www.photoneo.com/downloads/phoxi-control/)
  2. Select "PhoXi Control" in the dropdown menu and download the "Windows 10" installer
  3. Follow all of the prompts and select the "Full" install type
  4. The files should be installed to C:/Progam Files/Photoneo, which is the default option

## Installation

To build with Photoneo support: 

1. In the top level dir, create a `build` directory

2. Open the top level dir in the CMake GUI and set `build` as the build dir

3. Select the `WITH_PHOTONEO`checkbox to enable Photoneo support

4. Click "Configure"

5. Set `OpenCV_DIR` to the appropriate location. In our installation, this was in
   `C:/dev/opencv-4.7.0/install`

6. Click "Configure" - at this point, no red entries should appear

7. Click "Generate" then "Open Project" to open the project in Visual Studio

8. Be sure to build in Release Mode

9. You will need to add OpenCV to your `PATH` if it is not already, e.g.
   `C:\dev\opencv-4.7.0/build/bin/Release/`

10. You may need to copy some DLLs from Photoneo into the build folder - this can be found
    under `Photoneo/PhoXiContol-x.x.x/API/Bin

# Usage

TODO: Copy Ye Min's guidelines here.
