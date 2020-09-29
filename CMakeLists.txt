cmake_minimum_required(VERSION 3.10.0)

option(WITH_SPINNAKER "Use the Spinnaker SDK for increased camera support" OFF)

set(OpenGL_GL_PREFERENCE "GLVND")
set(OpenCV_DIR "" CACHE FILEPATH "Path to OpenCV 2")
set(Spinnaker_DIR "" CACHE FILEPATH "Path to the Spinnaker SDK")
set(PROJECT_NAME "scan3d-capture")

project(${PROJECT_NAME})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS forms)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(OpenCV 2 COMPONENTS core imgproc highgui calib3d features2d flann REQUIRED PATHS ${OpenCV_DIR})
find_package(OpenGL REQUIRED)
find_package(Qt5 COMPONENTS OpenGL REQUIRED)


add_executable(${PROJECT_NAME}
    src/AboutDialog.cpp
    src/Application.cpp
    src/CalibrationData.cpp
    src/CalibrationDialog.cpp
    src/CaptureDialog.cpp
#    src/cognex_util.cpp
    src/GLWidget.cpp
    src/ImageLabel.cpp
    src/im_util.cpp
    src/io_util.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/ProcessingDialog.cpp
    src/ProjectorWidget.cpp
    src/scan3d.cpp
    src/structured_light.cpp
    src/TreeModel.cpp
    src/VideoInput.cpp
    forms/AboutDialog.ui
    forms/CalibrationDialog.ui
    forms/CaptureDialog.ui
    forms/MainWindow.ui
    forms/ProcessingDialog.ui
    assets/assets.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE src forms ${OPENCV_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${OPENGL_LIBRARIES} Qt5::OpenGL)

if(WITH_SPINNAKER)
    find_package(Spinnaker REQUIRED)
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${SPINNAKER_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SPINNAKER_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_SPINNAKER)

    target_sources(${PROJECT_NAME} PRIVATE
        src/CameraConfigurationDialog.cpp
        forms/CameraConfigurationDialog.ui
    )

    MESSAGE(STATUS "Building with the Spinnaker SDK")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
