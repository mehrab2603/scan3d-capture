#ifndef __CAMERACONFIGURATIONDIALOG_HPP__
#define __CAMERACONFIGURATIONDIALOG_HPP__

#include <QDialog>
#include "Spinnaker.h"

#include "ui_CameraConfigurationDialog.h"

#include "VideoInput.hpp"

class CameraConfigurationDialog : public QDialog, public Ui::CameraConfigurationDialog
{
    Q_OBJECT

public:
    CameraConfigurationDialog(VideoInput & _video_input, QWidget * parent = 0, Qt::WindowFlags flags = Qt::WindowMaximizeButtonHint);
    ~CameraConfigurationDialog();

private:
    VideoInput & _video_input;
};

#endif  /* __CAMERACONFIGURATIONDIALOG_HPP__ */