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


#include "VideoInput.hpp"

#ifdef _MSC_VER
#   include <Dshow.h>
#endif

#ifdef Q_OS_MAC
#   include "VideoInput_QTkit.hpp"
#endif

#ifdef Q_OS_LINUX
#   include <unistd.h>
#   include <fcntl.h>
#   include <linux/videodev2.h>
#   include <sys/ioctl.h>
#   include <errno.h>
#   define V4L2_MAX_DEVICE_DRIVER_NAME 80
#   define V4L2_MAX_CAMERAS 8
#endif

#include <QApplication>
#include <QMetaType>
#include <QTime>
#include <QMap>

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <fstream>

#include <opencv2/imgproc/imgproc.hpp>

VideoInput::VideoInput(QObject  * parent): 
    QThread(parent),
    _camera_index(-1),
#ifdef USE_SPINNAKER
     _camera_name(""),
    _spinnaker_system(Spinnaker::System::GetInstance()),
    _spinnaker_camera(nullptr),
#endif
// Photoneo Support: Initialize Photoneo members / globals
#ifdef USE_PHOTONEO
    _camera_name(""),
    _photoneo_camera(nullptr),
#endif
    _video_capture(NULL),
    _init(false),
    _stop(false)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
}

VideoInput::~VideoInput()
{
    stop_camera(true);

#ifdef USE_SPINNAKER
    _spinnaker_system->ReleaseInstance();
#endif
// Photoneo Support: Teardown Photoneo connection
#ifdef USE_PHOTONEO
	// only disconnect the camera if it connected!
	if (_photoneo_camera) {
		_photoneo_camera->Disconnect();
	}
#endif
}

void VideoInput::run()
{
    _init = false;
    _stop = false;

    bool success = start_camera();

    _init = true;
    
    if (!success)
    {
        return;
    }

    int error_count = 0;
    int max_error = 10;
    int warmup = 10000;
    QTime timer;
    timer.start();

#ifdef USE_SPINNAKER
    while(_spinnaker_camera && !_stop && error_count<max_error)
    {
        Spinnaker::ImagePtr pResultImage = nullptr;

        try
        {
            pResultImage = _spinnaker_camera->GetNextImage(1000);

            if (pResultImage->IsIncomplete())
            {
                // Retrieve and print the image status description
                std::cout << "Image incomplete: " << Spinnaker::Image::GetImageStatusDescription(pResultImage->GetImageStatus()) << "..." << std::endl;
            }
            else
            {
                error_count = 0;

                Spinnaker::ImagePtr convertedImage = pResultImage->Convert(Spinnaker::PixelFormat_BGR8, Spinnaker::DIRECTIONAL_FILTER);
                
                unsigned int xPadding = static_cast<unsigned int>(convertedImage->GetXPadding());
                unsigned int yPadding = static_cast<unsigned int>(convertedImage->GetYPadding());
                unsigned int rowsize = static_cast<unsigned int>(convertedImage->GetWidth());
                unsigned int colsize = static_cast<unsigned int>(convertedImage->GetHeight());

                //image data contains padding. When allocating Mat container size, you need to account for the X,Y image data padding. 
                emit new_image(cv::Mat(colsize + yPadding, rowsize + xPadding, CV_8UC3, convertedImage->GetData(), convertedImage->GetStride()));
            }
        }
        catch (Spinnaker::Exception& e)
        {
            if (_spinnaker_camera->IsStreaming())
            {
                std::cerr << "Error captuing image: " << e.what() << std::endl;
                error_count++;
            }
        }
        
        try
        {
            if (pResultImage) pResultImage->Release();
        }
        catch(Spinnaker::Exception& e)
        {
            std::cerr << "Error releasing image: " << e.what() << std::endl;
        
    }
#endif
// Photoneo Support: Get images from the Photoneo sensor
#ifdef USE_PHOTONEO
	// TODO: Capture in 2D Image mode only?

    while(_photoneo_camera && !_stop && error_count<max_error)
    {
        _last_frame_trigger_time = std::chrono::steady_clock::now();
		int FrameID = _photoneo_camera->TriggerFrame();
		if (FrameID < 0)
		{
			// if negative number is returned, trigger was unsuccessful
			std::cout << "Trigger was unsuccessful! code=" << FrameID << std::endl;
		}
        else
        {
            std::cout << "Triggered Frame with ID: " << FrameID << std::endl;
        }

		pho::api::PFrame Frame = _photoneo_camera->GetFrame(pho::api::PhoXiTimeout::Infinity);

        if (Frame)
		{
            std::cout << "Frame Retrieved!" << std::endl;
		}
        else
        {
            std::cout << "Failed to retrieve the frame!" << std::endl;
        }


		int image_height = Frame->Texture.GetDimension(0);
		int image_width = Frame->Texture.GetDimension(1);

		// Frame-> Texture is a 32b Float that encodes intensity (1 channel only)
		cv::Mat Image = cv::Mat(image_height, image_width, CV_32FC1, Frame->Texture.GetDataPtr());

		// You can't use convertTo to change number of channels, so you need to first use cvt::Color
		// to go from 1 to 3 channels.
		cv::cvtColor(Image, Image, cv::COLOR_GRAY2RGB);

        // Normalizing with dynamic values leads to inconsistent illuminations across graycode-projected
        // scenes. Therefore, we must use fixed values for normalization. For now, we are using the maximum
        // and minimum values of 0 and 800, based on empirical observations alone.
        constexpr double minVal = 0.0;
        constexpr double maxVal = 1024.0;

		// Convert to 8UC3 with proper normalization - 8UC3 allows display by the camera preview.
        constexpr double scaling_factor = 255.0 / (maxVal - minVal);
        constexpr double increment = -255.0 * minVal / (maxVal - minVal);
		Image.convertTo(Image, CV_8UC3, scaling_factor, increment);

        emit new_image(Image);

        /* --------------------- Code for fetching and using RGB Texture ----------------------------------

		int image_height = Frame->TextureRGB.GetDimension(0);
		int image_width = Frame->TextureRGB.GetDimension(1);

		cv::Mat image = cv::Mat(image_height, image_width, CV_16UC3, Frame->TextureRGB.GetDataPtr());
        image = image / 4;
        image.convertTo(image, CV_8UC3);

        emit new_image(image);

        --------------------------------------------------------------------------------------------------*/
    }
#endif

    while(_video_capture && !_stop && error_count<max_error)
    {
        cv::Mat frame;
        if (_video_capture->read(frame))
        {   //ok
            error_count = 0;
            emit new_image(frame);
        }
        else
        {   //error
            if (timer.elapsed()>warmup) {error_count++;}
        }
    }

    //clean up
    stop_camera();
    QApplication::processEvents();
}

bool VideoInput::start_camera(void)
{
    int index = _camera_index;
    if (index<0)
    {
        return false;
    }

#ifdef USE_SPINNAKER
    if (_spinnaker_camera || _video_capture)
#elif USE_PHOTONEO
// Photoneo Support: check if Photoneo camera or video capture initialized
    if (_photoneo_camera || _video_capture)
#else
    if (_video_capture)
#endif
    {
        return false;
    }

    //set camera parameters (e.g. frame size)
    bool silent = true;

#ifdef _MSC_VER
    int CLASS = cv::CAP_DSHOW;
#endif
#ifdef Q_OS_MAC
    int CLASS = cv::CAP_QT;
#endif
#ifdef Q_OS_LINUX
    int CLASS = cv::CAP_V4L2;
#endif

#ifdef USE_SPINNAKER
    if (is_spinnaker_camera())
    {
        Spinnaker::CameraList cameraList = _spinnaker_system->GetCameras();
        _spinnaker_camera = cameraList.GetBySerial(_camera_name.substr(11));
        cameraList.Clear();

        std::cout << "Spinnaker camera opened: " << _camera_name << std::endl;
    }
    else
    {
#endif
// Photoneo Support: If camera is a photoneo camera, assign to photoneo camera object. This has an else clause down below
#ifdef USE_PHOTONEO
    if (is_photoneo_camera())
    {
		//Check if the PhoXi Control Software is running
		if (!_photoneo_system.isPhoXiControlRunning())
		{
			std::cout << "PhoXi Control Software is not running" << std::endl;
			throw "PhoXi Control Software is not running";
		}

		//Get List of available devices on the network
		std::vector <pho::api::PhoXiDeviceInformation> DeviceList = _photoneo_system.GetDeviceList();
		if (DeviceList.empty())
		{
			std::cout << "PhoXi Factory has found 0 devices" << std::endl;
			throw "PhoXi Factory has found 0 devices";
		}

		//Try to connect device opened in PhoXi Control, if any
		_photoneo_camera = _photoneo_system.CreateAndConnectFirstAttached();
		if (_photoneo_camera)
		{
			std::cout << "You have already PhoXi device opened in PhoXi Control, the API Example is connected to device: "
				<< (std::string) _photoneo_camera->HardwareIdentification << std::endl;
		}
		else
		{
			std::cout << "You have no PhoXi device opened in PhoXi Control, the API Example will try to connect to last device in device list" << std::endl;
			_photoneo_camera = _photoneo_system.CreateAndConnect(DeviceList.back().HWIdentification);
		}

		//Check if device was created
		if (!_photoneo_camera)
		{
			std::cout << "Your device was not created!" << std::endl;
			throw "Your device was not created!";
		}

		//Check if device is connected
		if (!(_photoneo_camera->isConnected()))
		{
			std::cout << "Your device is not connected" << std::endl;
			throw "Your device is not connected";
		}
    }
    else
    {
#endif
    //_video_capture = cvCaptureFromCAM(CLASS + index);
    _video_capture = std::make_shared<cv::VideoCapture>(CLASS + index);
    if(!_video_capture)
    {
        std::cerr << "camera open failed, index=" << index << std::endl;
        return false;
    }
#ifdef USE_SPINNAKER
    }

    if (is_spinnaker_camera())
    {
        configure_spinnaker_camera(index, silent);
    }
    else
    {
#endif
// Photoneo Support: End the else statement to open the Photoneo, configure it here
#ifdef USE_PHOTONEO
    }

    if (is_photoneo_camera())
    {
        configure_photoneo_camera(index, silent);
    }
    else
    {
#endif
#ifdef _MSC_VER
    configure_dshow(index,silent);
#endif
#ifdef Q_OS_MAC
    configure_quicktime(index,silent);
#endif
#ifdef Q_OS_LINUX
    configure_v4l2(index,silent);
#endif
#ifdef USE_SPINNAKER
    }
#endif
// Photoneo Support: End the other else statement to catch the photoneo config clause
#ifdef USE_PHOTONEO
    }
#endif
    return true;
}

void VideoInput::stop_camera(bool force)
{
#ifdef USE_SPINNAKER
    if (_spinnaker_camera)
    {
        _spinnaker_camera->EndAcquisition();
        _spinnaker_camera->DeInit();
        _spinnaker_camera = nullptr;
        _camera_name = "";
    }
#endif
// Photoneo Support: Stop the Photoneo camera acquisiition, tear it down
#ifdef USE_PHOTONEO
    if (_photoneo_camera)
    {
		_photoneo_camera->StopAcquisition();
        _photoneo_camera = (const pho::api::PPhoXi) nullptr;
        _camera_name = "";
    }
#endif
    if (_video_capture)
    {
#ifndef Q_OS_MAC //HACK: do not close on mac because it hangs the application
        _video_capture->release();
        _video_capture = nullptr;
#endif
    }
    if (_video_capture && force)
    {   //close no matter what
        _video_capture->release();
        _video_capture = nullptr;
    }
}

// TODO: Confgirue the Photoneo CAmera - seems like the place to actually set it up, also add it to the camera list
#ifdef USE_SPINNAKER

void VideoInput::configure_spinnaker_camera(int index, bool silent)
{
    try
    {
        // Retrieve TL device nodemap and print device information
        Spinnaker::GenApi::INodeMap& nodeMapTLDevice = _spinnaker_camera->GetTLDeviceNodeMap();

        // Print device info
        Spinnaker::GenApi::FeatureList_t features;
        const Spinnaker::GenApi::CCategoryPtr category = nodeMapTLDevice.GetNode("DeviceInformation");
        if (Spinnaker::GenApi::IsAvailable(category) && Spinnaker::GenApi::IsReadable(category))
        {
            category->GetFeatures(features);

            for (auto it = features.begin(); it != features.end(); ++it)
            {
                const Spinnaker::GenApi::CNodePtr pfeatureNode = *it;
                std::cout << pfeatureNode->GetName() << " : ";
                Spinnaker::GenApi::CValuePtr pValue = static_cast<Spinnaker::GenApi::CValuePtr>(pfeatureNode);
                std::cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
                std::cout << std::endl;
            }
        }
        else
        {
            std::cerr << "Error: Device control information not available." << std::endl;
        }

        // Initialize camera
        _spinnaker_camera->Init();

        // Load parameters from settings
        update_camera_parameters();

        // Start acquisition
        _spinnaker_camera->BeginAcquisition();
    }
    catch (Spinnaker::Exception& e)
    {
        std::cerr << "Error configuring Spinnaker camera: " << e.what() << std::endl;
    }
}

void VideoInput::update_camera_parameters()
{
    if (_spinnaker_camera)
    {
        bool isStreaming = _spinnaker_camera->IsStreaming();
        if (isStreaming) _spinnaker_camera->EndAcquisition();

        QSettings & config = APP->config;

        if (!configure_node("ExposureAuto", NodeType::Enum, "Off"))
        {
            std::cerr << "Error: Could not turn off camera Auto Exposure" << std::endl;
        }

        if (!configure_node("GainAuto", NodeType::Enum, "Off"))
        {
            std::cerr << "Error: Could not turn off camera Auto Gain" << std::endl;
        }

        if (!configure_node("BalanceWhiteAuto", NodeType::Enum, "Off"))
        {
            std::cerr << "Error: Could not turn off camera Auto White Balance" << std::endl;
        }

        if (!configure_node("AcquisitionMode", NodeType::Enum, "Continuous"))
        {
            std::cerr << "Error: Could not set camera Acquision Mode" << std::endl;
            return;
        }

        if (!configure_node("BalanceRatioSelector", NodeType::Enum, "Red"))
        {
            config.setValue(CAMERA_BALANCE_RED_CONFIG, CAMERA_BALANCE_RED_DEFAULT);
            std::cerr << "Error: Could not set camera Balance (Red)" << std::endl;
        }
        else
        {
            if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->BalanceRatio) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->BalanceRatio))
            {
                _spinnaker_camera->BalanceRatio.SetValue(config.value(CAMERA_BALANCE_RED_CONFIG, CAMERA_BALANCE_RED_DEFAULT).toDouble());   
            }
            else
            {
                config.setValue(CAMERA_BALANCE_RED_CONFIG, CAMERA_BALANCE_RED_DEFAULT);
                std::cerr << "Error: Could not set camera Balance (Red)" << std::endl;
            }
        }

        if (!configure_node("BalanceRatioSelector", NodeType::Enum, "Blue"))
        {
            config.setValue(CAMERA_BALANCE_BLUE_CONFIG, CAMERA_BALANCE_BLUE_DEFAULT);
            std::cerr << "Error: Could not set camera Balance (Blue)" << std::endl;
        }
        else
        {
            if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->BalanceRatio) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->BalanceRatio))
            {
                _spinnaker_camera->BalanceRatio.SetValue(config.value(CAMERA_BALANCE_BLUE_CONFIG, CAMERA_BALANCE_BLUE_DEFAULT).toDouble());   
            }
            else
            {
                config.setValue(CAMERA_BALANCE_BLUE_CONFIG, CAMERA_BALANCE_BLUE_DEFAULT);
                std::cerr << "Error: Could not set camera Balance (Blue)" << std::endl;
            }
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->Height) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->Height))
        {
            _spinnaker_camera->Height.SetValue(config.value(CAMERA_HEIGHT_CONFIG, CAMERA_HEIGHT_DEFAULT).toUInt());   
        }
        else
        {
            config.setValue(CAMERA_HEIGHT_CONFIG, CAMERA_HEIGHT_DEFAULT);
            std::cerr << "Error: Could not set camera Height" << std::endl;
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->Width) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->Width))
        {
            _spinnaker_camera->Width.SetValue(config.value(CAMERA_WIDTH_CONFIG, CAMERA_WIDTH_DEFAULT).toUInt());   
        }
        else
        {
            config.setValue(CAMERA_WIDTH_CONFIG, CAMERA_WIDTH_DEFAULT);
            std::cerr << "Error: Could not set camera Width" << std::endl;
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->OffsetX) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->OffsetX))
        {
            _spinnaker_camera->OffsetX.SetValue(config.value(CAMERA_OFFSET_X_CONFIG, CAMERA_OFFSET_X_DEFAULT).toUInt());   
        }
        else
        {
            config.setValue(CAMERA_OFFSET_X_CONFIG, CAMERA_OFFSET_X_DEFAULT);
            std::cerr << "Error: Could not set camera X Offset" << std::endl;
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->OffsetY) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->OffsetY))
        {
            _spinnaker_camera->OffsetY.SetValue(config.value(CAMERA_OFFSET_Y_CONFIG, CAMERA_OFFSET_Y_DEFAULT).toUInt());   
        }
        else
        {
            config.setValue(CAMERA_OFFSET_Y_CONFIG, CAMERA_OFFSET_Y_DEFAULT);
            std::cerr << "Error: Could not set camera Y Offset" << std::endl;
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->Gain) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->Gain))
        {
            _spinnaker_camera->Gain.SetValue(config.value(CAMERA_GAIN_CONFIG, CAMERA_GAIN_DEFAULT).toDouble());   
        }
        else
        {
            config.setValue(CAMERA_GAIN_CONFIG, CAMERA_GAIN_DEFAULT);
            std::cerr << "Error: Could not set camera Gain" << std::endl;
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->BlackLevel) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->BlackLevel))
        {
            _spinnaker_camera->BlackLevel.SetValue(config.value(CAMERA_BLACK_LEVEL_CONFIG, CAMERA_BLACK_LEVEL_DEFAULT).toDouble());   
        }
        else
        {
            config.setValue(CAMERA_BLACK_LEVEL_CONFIG, CAMERA_BLACK_LEVEL_DEFAULT);
            std::cerr << "Error: Could not set camera Brightness" << std::endl;
        }

        if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->ExposureTime) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->ExposureTime))
        {
            double exposureValue = std::clamp(config.value(CAMERA_EXPOSURE_TIME_CONFIG, CAMERA_EXPOSURE_TIME_DEFAULT).toDouble(), _spinnaker_camera->ExposureTime.GetMin(), _spinnaker_camera->ExposureTime.GetMax());
            config.setValue(CAMERA_EXPOSURE_TIME_CONFIG, exposureValue);

            _spinnaker_camera->ExposureTime.SetValue(exposureValue);   
        }
        else
        {
            config.setValue(CAMERA_EXPOSURE_TIME_CONFIG, CAMERA_EXPOSURE_TIME_DEFAULT);
            std::cerr << "Error: Could not set camera Exposure Time" << std::endl;
        }

        if (!configure_node("AcquisitionFrameRateEnable", NodeType::Bool, "true"))
        {
            config.setValue(CAMERA_FRAME_RATE_CONFIG, CAMERA_FRAME_RATE_DEFAULT);
            std::cerr << "Error: Could not set camera Frame Rate Enable" << std::endl;
        }
        else
        {
            if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->AcquisitionFrameRate) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->AcquisitionFrameRate))
            {
                double frameRateValue = std::clamp(config.value(CAMERA_FRAME_RATE_CONFIG, CAMERA_FRAME_RATE_DEFAULT).toDouble(), _spinnaker_camera->AcquisitionFrameRate.GetMin(), _spinnaker_camera->AcquisitionFrameRate.GetMax());
                config.setValue(CAMERA_FRAME_RATE_CONFIG, frameRateValue);

                _spinnaker_camera->AcquisitionFrameRate.SetValue(frameRateValue);   
            }
            else
            {
                config.setValue(CAMERA_FRAME_RATE_CONFIG, CAMERA_FRAME_RATE_DEFAULT);
                std::cerr << "Error: Could not set camera Frame Rate" << std::endl;
            }
        }

        if (!configure_node("GammaEnable", NodeType::Bool, "true"))
        {
            config.setValue(CAMERA_GAMMA_CONFIG, CAMERA_GAMMA_DEFAULT);
            std::cerr << "Error: Could not set camera Gamma Enable" << std::endl;
        }
        else
        {
            if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->Gamma) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->Gamma))
            {
                double gammaValue = std::clamp(config.value(CAMERA_GAMMA_CONFIG, CAMERA_GAMMA_DEFAULT).toDouble(), _spinnaker_camera->Gamma.GetMin(), _spinnaker_camera->Gamma.GetMax());
                config.setValue(CAMERA_GAMMA_CONFIG, gammaValue);

                _spinnaker_camera->Gamma.SetValue(gammaValue);   
            }
            else
            {
                config.setValue(CAMERA_GAMMA_CONFIG, CAMERA_GAMMA_DEFAULT);
                std::cerr << "Error: Could not set camera Gamma" << std::endl;
            }
        }

        if (!configure_node("SaturationEnable", NodeType::Bool, "true"))
        {
            config.setValue(CAMERA_SATURATION_CONFIG, CAMERA_SATURATION_DEFAULT);
            std::cerr << "Error: Could not set camera Saturation Enable" << std::endl;
        }
        else
        {
            if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->Saturation) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->Saturation))
            {
                double saturationValue = std::clamp(config.value(CAMERA_SATURATION_CONFIG, CAMERA_SATURATION_DEFAULT).toDouble(), _spinnaker_camera->Saturation.GetMin(), _spinnaker_camera->Saturation.GetMax());
                config.setValue(CAMERA_SATURATION_CONFIG, saturationValue);

                _spinnaker_camera->Saturation.SetValue(saturationValue);   
            }
            else
            {
                config.setValue(CAMERA_SATURATION_CONFIG, CAMERA_SATURATION_DEFAULT);
                std::cerr << "Error: Could not set camera Sharpening" << std::endl;
            }
        }

        if (!configure_node("SharpeningEnable", NodeType::Bool, "true"))
        {
            config.setValue(CAMERA_SHARPNESS_CONFIG, CAMERA_SHARPNESS_DEFAULT);
            std::cerr << "Error: Could not set camera Sharpening Enable" << std::endl;
        }
        else
        {
            if (Spinnaker::GenApi::IsReadable(_spinnaker_camera->Sharpening) && Spinnaker::GenApi::IsWritable(_spinnaker_camera->Sharpening))
            {
                double sharpeningValue = std::clamp(config.value(CAMERA_SHARPNESS_CONFIG, CAMERA_SHARPNESS_DEFAULT).toDouble(), _spinnaker_camera->Sharpening.GetMin(), _spinnaker_camera->Sharpening.GetMax());
                config.setValue(CAMERA_SHARPNESS_CONFIG, sharpeningValue);

                _spinnaker_camera->Sharpening.SetValue(sharpeningValue);   
            }
            else
            {
                config.setValue(CAMERA_SHARPNESS_CONFIG, CAMERA_SHARPNESS_DEFAULT);
                std::cerr << "Error: Could not set camera Sharpening" << std::endl;
            }
        }

        if (isStreaming) _spinnaker_camera->BeginAcquisition();
    }
}

bool VideoInput::configure_node(std::string nodeName, NodeType nodeType, std::string value)
{
    if (_spinnaker_camera)
    {
        Spinnaker::GenApi::INodeMap& nodeMap = _spinnaker_camera->GetNodeMap();

        if (nodeType == NodeType::Enum)
        {
            Spinnaker::GenApi::CEnumerationPtr enumerationPtr = nodeMap.GetNode(nodeName.c_str());

            if (!Spinnaker::GenApi::IsAvailable(enumerationPtr) || !Spinnaker::GenApi::IsWritable(enumerationPtr))
            {
                return false;
            }
            else
            {
                // Retrieve entry node from enumeration node
                Spinnaker::GenApi::CEnumEntryPtr enumEntryPtr = enumerationPtr->GetEntryByName(value.c_str());

                if (!Spinnaker::GenApi::IsAvailable(enumEntryPtr) || !Spinnaker::GenApi::IsReadable(enumEntryPtr))
                {
                    return false;
                }
                else
                {
                    // Retrieve integer value from entry node
                    const int64_t valueCode = enumEntryPtr->GetValue();

                    // Set integer value from entry node as new value of enumeration node
                    enumerationPtr->SetIntValue(valueCode);

                    return true;
                }
            }
        }
        else if (nodeType == NodeType::Bool)
        {
            Spinnaker::GenApi::CBooleanPtr booleanPtr = nodeMap.GetNode(nodeName.c_str());

            if (!Spinnaker::GenApi::IsAvailable(booleanPtr) || !Spinnaker::GenApi::IsReadable(booleanPtr))
            {
                return false;
            }

            booleanPtr->SetValue(value == "true" ? true : false);
            return true;
        }
    }
    else
    {
        return false;
    }

    return false;
}

QStringList VideoInput::list_devices_spinnaker()
{
    QStringList list;
    Spinnaker::CameraList cameraList = _spinnaker_system->GetCameras();

    char deviceName[255];

    for (unsigned int i = 0; i < cameraList.GetSize(); i++)
    {
        Spinnaker::GenApi::CStringPtr ptrStringSerial = cameraList.GetByIndex(i)->GetTLDeviceNodeMap().GetNode("DeviceSerialNumber");
        if (Spinnaker::GenApi::IsAvailable(ptrStringSerial) && Spinnaker::GenApi::IsReadable(ptrStringSerial))
        {
            sprintf(deviceName, "Spinnaker: %s", ptrStringSerial->GetValue().c_str());
            list.append(deviceName);
        }
    }

    cameraList.Clear();
    return list;
}

#endif
// Photoneo Support: Camera Configuration, Make Camera Appear in List
// TODO: Support updating camera parameters
#ifdef USE_PHOTONEO
void VideoInput::configure_photoneo_camera(int index, bool silent)
{
        // Initialize camera
		if (_photoneo_camera->isAcquiring())
		{
			//Stop acquisition to change trigger mode
			_photoneo_camera->StopAcquisition();
		}

		_photoneo_camera->TriggerMode = pho::api::PhoXiTriggerMode::Software;
		std::cout << "Software trigger mode was set" << std::endl;
		_photoneo_camera->ClearBuffer();
		std::cout << "Buffer Cleared" << std::endl;
		_photoneo_camera->StartAcquisition();
		std::cout << "Acqusition Started" << std::endl;
		if (!_photoneo_camera->isAcquiring())
		{
			std::cout << "Your device could not start acquisition!" << std::endl;
			throw "Your device could not start acquisition!";
		}
}

QStringList VideoInput::list_devices_photoneo()
{
    QStringList list;
	std::vector <pho::api::PhoXiDeviceInformation> DeviceList = _photoneo_system.GetDeviceList();

    char deviceName[255];

    for (unsigned int i = 0; i < DeviceList.size(); i++)
    {
        sprintf(deviceName, "Photoneo: %s", DeviceList[i].Name);
        list.append(deviceName);
    }

    return list;
}

#endif

void VideoInput::waitForStart(void)
{
    while (isRunning() && !_init)
    {
        QApplication::processEvents();
    }
}

void VideoInput::setImageSize(size_t width, size_t height)
{
#ifdef USE_SPINNAKER
  if (_spinnaker_camera)
  {
      //TODO: implementation
  }
#endif
// Photoneo Support: Set image size for Photoneo - likely not needed?
#ifdef USE_PHOTONEO
  if (_photoneo_camera)
  {
      //TODO: implementation
  }
#endif
  if (_video_capture)
  {
    _video_capture->set(cv::CAP_PROP_FRAME_WIDTH, width);
    _video_capture->set(cv::CAP_PROP_FRAME_HEIGHT, height);
    std::cerr << "setImageSize: " << width << "x" << height << std::endl;
  }
}

QStringList VideoInput::list_devices(void)
{
    QStringList list;
    bool silent = true;
#ifdef _MSC_VER
    list = list_devices_dshow(silent);
#endif
#ifdef Q_OS_MAC
    list = list_devices_quicktime(silent);
#endif
#ifdef Q_OS_LINUX
    list = list_devices_v4l2(silent);
#endif
#ifdef USE_SPINNAKER
    list += list_devices_spinnaker();
#endif
// Photoneo Support: Add photoneo camera to the list
#ifdef USE_PHOTONEO
    list += list_devices_photoneo();
#endif

    return list;
}

QStringList VideoInput::list_device_resolutions(int index)
{
    QStringList list;
    bool silent = true;
#ifdef _MSC_VER
    list = list_device_resolutions_dshow(index, silent);
#endif
#ifdef Q_OS_MAC
    list = list_device_resolutions_quicktime(index, silent);
#endif
#ifdef Q_OS_LINUX
    list = list_device_resolutions_v4l2(index, silent);
#endif

    return list;
}

/*
   listDevices_dshow() is based on videoInput library by Theodore Watson:
   http://muonics.net/school/spring05/videoInput/

   Below is the original copyright
*/

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

//////////////////////////////////////////////////////////
//Written by Theodore Watson - theo.watson@gmail.com    //
//Do whatever you want with this code but if you find   //
//a bug or make an improvement I would love to know!    //
//                                                      //
//Warning This code is experimental                     //
//use at your own risk :)                               //
//////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*                     Shoutouts

Thanks to:

           Dillip Kumar Kara for crossbar code.
           Zachary Lieberman for getting me into this stuff
           and for being so generous with time and code.
           The guys at Potion Design for helping me with VC++
           Josh Fisher for being a serious C++ nerd :)
           Golan Levin for helping me debug the strangest
           and slowest bug in the world!

           And all the people using this library who send in
           bugs, suggestions and improvements who keep me working on
           the next version - yeah thanks a lot ;)

*/
/////////////////////////////////////////////////////////

QStringList VideoInput::list_devices_dshow(bool silent)
{
    if (!silent) { printf("\nVIDEOINPUT SPY MODE!\n\n"); }

    QStringList list;

#ifdef _MSC_VER
    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    int deviceCounter = 0;
    
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
        CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
        reinterpret_cast<void**>(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the video capture category.
        hr = pDevEnum->CreateClassEnumerator(
            CLSID_VideoInputDeviceCategory,
            &pEnum, 0);

        if(hr == S_OK){

            if (!silent) { printf("SETUP: Looking For Capture Devices\n"); }
            IMoniker *pMoniker = NULL;

            while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
            {
                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));

                if (FAILED(hr)){
                    pMoniker->Release();
                    continue;  // Skip this one, maybe the next one will work.
                }

                 // Find the description or friendly name.
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"Description", &varName, 0);

                if (FAILED(hr)) hr = pPropBag->Read(L"FriendlyName", &varName, 0);

                if (SUCCEEDED(hr)){

                    hr = pPropBag->Read(L"FriendlyName", &varName, 0);

                    char deviceName[255] = {0};

                    int count = 0;
                    int maxLen = sizeof(deviceName)/sizeof(deviceName[0]) - 2;
                    while( varName.bstrVal[count] != 0x00 && count < maxLen) {
                        deviceName[count] = (char)varName.bstrVal[count];
                        count++;
                    }
                    deviceName[count] = 0;
                    list.append(deviceName);

                    if (!silent) { printf("SETUP: %i) %s \n",deviceCounter, deviceName); }
                }

                pPropBag->Release();
                pPropBag = NULL;

                pMoniker->Release();
                pMoniker = NULL;

                deviceCounter++;
            }

            pDevEnum->Release();
            pDevEnum = NULL;

            pEnum->Release();
            pEnum = NULL;
        }

         if (!silent) { printf("SETUP: %i Device(s) found\n\n", deviceCounter); }
    }
#endif //_MSC_VER

    return list;
}

void VideoInput::configure_dshow(int index, bool silent)
{
    QStringList resList = list_device_resolutions_dshow(index, silent);
    unsigned int pixCount = 0, cols = 0, rows = 0;
    foreach (auto resString, resList)
    {
        QStringList res = resString.split('x');
        if (res.length()<2) { continue; }

        unsigned int curCols = res.at(0).toUInt();
        unsigned int curRows = res.at(1).toUInt();
        unsigned int curPixCount = curCols*curRows;
        if (curPixCount>pixCount)
        {
            pixCount = curPixCount;
            cols = curCols;
            rows = curRows;
        }
    }

    if (pixCount)
    {
        _video_capture->set(cv::CAP_PROP_FRAME_WIDTH, cols);
        _video_capture->set(cv::CAP_PROP_FRAME_HEIGHT, rows);
    }
}

QStringList VideoInput::list_device_resolutions_dshow(int index, bool silent)
{
    QStringList list;

#ifdef _MSC_VER
    //create System Device Enumerator Service
    ICreateDevEnum *pSysDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void**>(&pSysDevEnum));
    if (!SUCCEEDED(hr)) { return list; }

    //create an enumerator for the video capture category.
    IEnumMoniker *pDevEnum = NULL;
    hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pDevEnum, 0);
    if (hr!=S_OK) { return list; }

    //enumerate devices
    int deviceCounter = 0;
    IMoniker *pMoniker = NULL;
    while (deviceCounter<=index && pDevEnum->Next(1, &pMoniker, NULL)==S_OK)
    {
        //we don't need the properties but follow the same logic as when listing devices to keep the same ordering
        IPropertyBag *pPropBag;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));
        if (!FAILED(hr))
        {   //ok
            deviceCounter++;
        }

        pPropBag->Release();
        pMoniker->Release();
    }

    if (deviceCounter<=index)
    {   //error: not enough devices
        pSysDevEnum->Release();
        pDevEnum->Release();
        return list;
    }

    //device found: search available resolutions
    IBaseFilter * pFilter = NULL;
    hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
    if (!SUCCEEDED(hr))
    {   //error
        return list;
    }

    if (!silent) { printf("filter created!\n"); }

    //create pin enumerator
    IEnumPins *pPinEnum = NULL;
    hr = pFilter->EnumPins(&pPinEnum);
    if (!SUCCEEDED(hr))
    {   //error
        pFilter->Release();
        return list;
    }

    //resolutions map
    QMap<QString,bool> resMap;

    //enumerate pins
    IPin *pPin = NULL;
    while (pPinEnum->Next(1, &pPin, 0)==S_OK)
    {
        //query pin direction
        PIN_DIRECTION PinDirThis;
        hr = pPin->QueryDirection(&PinDirThis);
        if (!SUCCEEDED(hr) || PinDirThis!=PINDIR_OUTPUT)
        {   //skip
            pPin->Release();
            continue;
        }

        if (!silent)
        {   //print pin name
            PIN_INFO pi;
            hr = pPin->QueryPinInfo(&pi);
            if (SUCCEEDED(hr))
            {
                CLSID clsid;
                hr = pi.pFilter->GetClassID(&clsid);
                if (SUCCEEDED(hr))
                {
                    TCHAR str[MAX_PIN_NAME];

                    std::wstring pinNameWideString(pi.achName);
                    std::string pinNameString(pinNameWideString.begin(), pinNameWideString.end());

                    StringCchCopy(str, NUMELMS(str), pinNameString.c_str());
                    printf(" Pin name %S\n", str);
                }
                if (pi.pFilter) { pi.pFilter->Release(); }
            }
        }

        //prepare for querying pin category
        GUID pinCategory;
        IKsPropertySet *pKs = NULL;
        hr = pPin->QueryInterface(IID_PPV_ARGS(&pKs));
        if (FAILED(hr)) 
        {   //error
            pPin->Release();
            continue; 
        }

        //query pin category.
        DWORD cbReturned = 0;
        hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pinCategory, sizeof(GUID), &cbReturned);
        if (!SUCCEEDED(hr))
        {   //error
            pKs->Release();
            pPin->Release();
            continue; 
        }
        if (!silent)
        {
            printf(" Pin category %s\n", (pinCategory==PIN_CATEGORY_STILL ? "STILL" : (pinCategory==PIN_CATEGORY_CAPTURE ? "CAPTURE" : "OTHER")));
        }

        if (pinCategory!=PIN_CATEGORY_CAPTURE)
        {   //skip
            pKs->Release();
            pPin->Release();
            continue;
        }

        //output pin found: enumerate media types
        IEnumMediaTypes * mediaTypesEnumerator = NULL;
        hr = pPin->EnumMediaTypes(&mediaTypesEnumerator);
        if (FAILED(hr)) 
        { 
            pKs->Release();
            pPin->Release();
            continue; 
        }

        //enumerate media types
        AM_MEDIA_TYPE* mediaType = NULL;  
        while (mediaTypesEnumerator->Next(1, &mediaType, NULL)==S_OK)
        {
            if ((mediaType->formattype == FORMAT_VideoInfo) &&
                (mediaType->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
                (mediaType->pbFormat != NULL))
            {
                VIDEOINFOHEADER* videoInfoHeader = (VIDEOINFOHEADER*)mediaType->pbFormat;
                char code[5] = {0, 0, 0, 0, 0};
                *reinterpret_cast<LONG*>(code) = videoInfoHeader->bmiHeader.biCompression;
                if (!silent)
                {
                    /*printf(" w %d h %d type %s\n", 
                            videoInfoHeader->bmiHeader.biWidth,  // Supported width
                            videoInfoHeader->bmiHeader.biHeight, // Supported height
                            (videoInfoHeader->bmiHeader.biCompression==BI_RGB? "RGB" :
                                (videoInfoHeader->bmiHeader.biCompression==BI_BITFIELDS ? "BITFIELDS" : code)
                            )
                          );*/
                }//if !silent

                QString res = QString("%1x%2").arg(videoInfoHeader->bmiHeader.biWidth).arg(videoInfoHeader->bmiHeader.biHeight);
                resMap[res] = true;
            }

            //clean
            if (mediaType->cbFormat) { CoTaskMemFree(mediaType->pbFormat); }
            if (mediaType->pUnk)     { mediaType->pUnk->Release();         }
            CoTaskMemFree(mediaType);
        }//while (mediaTypesEnumerator->Next)
                                 
        mediaTypesEnumerator->Release();
        pKs->Release();
        pPin->Release();
    }//while (pPinEnum->Next)

    //cleanup
    pPinEnum->Release();
    pFilter->Release();
    pSysDevEnum->Release();
    pDevEnum->Release();

    if (!silent) { printf("SETUP: device %i resolutions queried\n\n", index); }

    foreach (auto res, resMap.keys())
    {
        if (!silent) { printf("res %s\n", qPrintable(res)); }
        list.append(res);
    }
    
#endif //_MSC_VER

    return list;
}

QStringList VideoInput::list_devices_quicktime(bool silent)
{
    if (!silent) { std::cerr << "\n[list_devices_quicktime]\n\n"; }

    QStringList list;

#ifdef Q_OS_MAC
    list = list_devices_qtkit(silent);
#endif //Q_OS_MAC

    return list;
}

void VideoInput::configure_quicktime(int index, bool silent)
{
}

QStringList VideoInput::list_device_resolutions_quicktime(int index, bool silent)
{
    QStringList list;
    return list;
}

QStringList VideoInput::list_devices_v4l2(bool silent)
{
    if (!silent) { std::cerr << "\n[list_devices_v4l2]\n\n"; }

    QStringList list;

#ifdef Q_OS_LINUX

    /* Simple test program: Find number of Video Sources available.
       Start from 0 and go to MAX_CAMERAS while checking for the device with that name.
       If it fails on the first attempt of /dev/video0, then check if /dev/video is valid.
       Returns the global numCameras with the correct value (we hope) */

    int CameraNumber = 0;
    char deviceName[V4L2_MAX_DEVICE_DRIVER_NAME];

    while(CameraNumber < V4L2_MAX_CAMERAS) 
    {
        /* Print the CameraNumber at the end of the string with a width of one character */
        sprintf(deviceName, "/dev/video%1d", CameraNumber);

        /* Test using an open to see if this new device name really does exists. */
        int deviceHandle = open(deviceName, O_RDONLY);
        if (deviceHandle != -1)
        {
            /* This device does indeed exist - add it to the total so far */
            list.append(deviceName);
        }

        close(deviceHandle);

        /* Set up to test the next /dev/video source in line */
        CameraNumber++;
    } /* End while */

    if (!silent) { fprintf(stderr, "v4l numCameras %d\n", list.length()); }

#endif //Q_OS_LINUX

    return list;
}

void VideoInput::configure_v4l2(int index, bool silent)
{
#ifdef Q_OS_LINUX

    cv::Size requestSize(0,0);
    int CameraNumber = index;

    /* Print the CameraNumber at the end of the string with a width of one character */
    char deviceName[V4L2_MAX_DEVICE_DRIVER_NAME];
    sprintf(deviceName, "/dev/video%1d", CameraNumber);

    /* Test using an open to see if this new device name really does exists. */
    int deviceHandle = open(deviceName, O_RDONLY);
    if (deviceHandle != -1)
    {
        //find the maximum resolution
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index = 0;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while (ioctl(deviceHandle, VIDIOC_ENUM_FMT, &fmtdesc)==0 && fmtdesc.index!=EINVAL)
        {
            if (!silent) { fprintf(stderr, "v4l cam %d, pixel_format %d: %s\n", CameraNumber, fmtdesc.index, fmtdesc.description); }
            
            struct v4l2_frmsizeenum frmsizeenum;
            frmsizeenum.index = 0;
            frmsizeenum.pixel_format = fmtdesc.pixelformat;

            while (ioctl(deviceHandle, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum)==0 && frmsizeenum.index!=EINVAL)
            {
                cv::Size size;
                if (frmsizeenum.type==V4L2_FRMSIZE_TYPE_DISCRETE)
                {
                    size = cv::Size(frmsizeenum.discrete.width, frmsizeenum.discrete.height);
                }
                else
                {
                    size = cv::Size(frmsizeenum.stepwise.max_width, frmsizeenum.stepwise.min_height);
                }

                if (requestSize.width<size.width)
                {
                    requestSize = size;
                }
                
                if (!silent) { fprintf(stderr, "v4l cam %d, supported size w=%d h=%d\n", CameraNumber, size.width, size.height); }

                ++frmsizeenum.index;
            }
            ++fmtdesc.index;
        }
    }
    close(deviceHandle);

    if (_video_capture && requestSize.width>0)
    {
        if (!silent) { fprintf(stderr, " *** v4l cam %d, selected size w=%d h=%d\n", CameraNumber, requestSize.width, requestSize.height); }
        
        _video_capture->set(cv::CAP_PROP_FRAME_WIDTH, requestSize.width);
        _video_capture->set(cv::CAP_PROP_FRAME_HEIGHT, requestSize.height);
    }

#endif //Q_OS_LINUX
}

QStringList VideoInput::list_device_resolutions_v4l2(int index, bool silent)
{
    QStringList list;
#ifdef Q_OS_LINUX
#endif //Q_OS_LINUX
    return list;
}
