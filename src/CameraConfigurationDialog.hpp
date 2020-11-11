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

public slots:
    void on_camera_height_spin_valueChanged(int i);
    void on_camera_width_spin_valueChanged(int i);
    void on_camera_offset_x_spin_valueChanged(int i);
    void on_camera_offset_y_spin_valueChanged(int i);
    void on_camera_brightness_slider_valueChanged(int i);
    void on_camera_brightness_spin_valueChanged(double i);
    void on_camera_gain_slider_valueChanged(int i);
    void on_camera_gain_spin_valueChanged(double i);
    void on_camera_exposure_time_slider_valueChanged(int i);
    void on_camera_exposure_time_spin_valueChanged(double i);
    void on_camera_balance_red_slider_valueChanged(int i);
    void on_camera_balance_red_spin_valueChanged(double i);
    void on_camera_balance_blue_slider_valueChanged(int i);
    void on_camera_balance_blue_spin_valueChanged(double i);
    void on_camera_frame_rate_slider_valueChanged(int i);
    void on_camera_frame_rate_spin_valueChanged(double i);
    void on_camera_sharpness_slider_valueChanged(int i);
    void on_camera_sharpness_spin_valueChanged(double i);
    void on_camera_hue_slider_valueChanged(int i);
    void on_camera_hue_spin_valueChanged(double i);
    void on_camera_saturation_slider_valueChanged(int i);
    void on_camera_saturation_spin_valueChanged(double i);
    void on_camera_gamma_slider_valueChanged(int i);
    void on_camera_gamma_spin_valueChanged(double i);
private:
    void update_ui(bool update_resolution_spins);
    template<typename T1, typename T2>
    void setup_slider_spin_pair(QSlider * slider, T2 * spin, std::string configString, T1 defaultValue, T1 minValue, T1 maxValue, double maxOffset=0.1);
    template<typename T>
    void apply_slider_spin_pair(QSlider * slider, QDoubleSpinBox * spin, T i, std::string config);  

    VideoInput & _video_input;
};

#endif  /* __CAMERACONFIGURATIONDIALOG_HPP__ */