/*
Copyright (c) 2014, Daniel Moreno and Gabriel Taubin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Brown University nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DANIEL MORENO AND GABRIEL TAUBIN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __VIDEOINPUT_HPP__
#define __VIDEOINPUT_HPP__

#include <QThread>
#include <QStringList>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef USE_SPINNAKER
#   include "Spinnaker.h"
#endif

class VideoInput : public QThread
{
    Q_OBJECT

public:
    VideoInput(QObject * parent = 0);
    ~VideoInput();

    inline void stop(void) {_stop = true;}
    inline void set_camera_index(int index) {_camera_index = index;}
    inline int get_camera_index(void) const {return _camera_index;}

#ifdef USE_SPINNAKER
    inline void set_camera_name(std::string name) {_camera_name = name;}
    inline std::string get_camera_name(void) const {return _camera_name;}
    inline bool is_spinnaker_camera(void) const {return _camera_name != "" && _camera_name.rfind("Spinnaker", 0) == 0;}
    // std::shared_ptr<FlyCapture2::FC2Config> get_camera_config();
#endif

    void setImageSize(size_t width, size_t height);

    QStringList list_devices(void);
    QStringList list_device_resolutions(int index);

    void waitForStart(void);

signals:
    void new_image(cv::Mat image);

protected:
    virtual void run();

private:
    QStringList list_devices_dshow(bool silent);
    QStringList list_devices_quicktime(bool silent);
    QStringList list_devices_v4l2(bool silent);

    void configure_dshow(int index, bool silent);
    void configure_quicktime(int index, bool silent);
    void configure_v4l2(int index, bool silent);

    QStringList list_device_resolutions_dshow(int index, bool silent);
    QStringList list_device_resolutions_quicktime(int index, bool silent);
    QStringList list_device_resolutions_v4l2(int index, bool silent);

    bool start_camera(void);
    void stop_camera(bool force = false);

#ifdef USE_SPINNAKER
    void configure_spinnaker_camera(int index, bool silent);
#endif

private:
    int _camera_index;
    std::string _camera_name;
#ifdef USE_SPINNAKER
    Spinnaker::SystemPtr _spinnaker_system;
    Spinnaker::CameraPtr _spinnaker_camera;
#endif
    CvCapture * _video_capture;
    volatile bool _init;
    volatile bool _stop;
};

#endif  /* __VIDEOINPUT_HPP__ */
