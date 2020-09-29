#include "CameraConfigurationDialog.hpp"

CameraConfigurationDialog::CameraConfigurationDialog(VideoInput & _video_input, QWidget * parent, Qt::WindowFlags flags): 
    QDialog(parent, flags),
    _video_input(_video_input)
{
    setupUi(this);

    while (!_video_input.isRunning())
    {
        _video_input.start();
        _video_input.waitForStart();
    }
}

CameraConfigurationDialog::~CameraConfigurationDialog()
{
    
}