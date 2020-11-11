#include "CameraConfigurationDialog.hpp"

CameraConfigurationDialog::CameraConfigurationDialog(VideoInput & _video_input, QWidget * parent, Qt::WindowFlags flags): 
    QDialog(parent, flags),
    _video_input(_video_input)
{
    setupUi(this);

    Spinnaker::CameraPtr camera = _video_input.get_camera();

    if (!camera)
    {
        return;
    }

    update_ui(true);
}

CameraConfigurationDialog::~CameraConfigurationDialog()
{

}

void CameraConfigurationDialog::update_ui(bool update_resolution_spins)
{
    Spinnaker::CameraPtr camera = _video_input.get_camera();

    if (update_resolution_spins)
    {
        setup_slider_spin_pair<int, QSpinBox>(nullptr, camera_height_spin, CAMERA_HEIGHT_CONFIG, CAMERA_HEIGHT_DEFAULT, camera->Height.GetMin(), camera->Height.GetMax(), 0);
        setup_slider_spin_pair<int, QSpinBox>(nullptr, camera_width_spin, CAMERA_WIDTH_CONFIG, CAMERA_WIDTH_DEFAULT, camera->Width.GetMin(), camera->Width.GetMax(), 0);
    }

    setup_slider_spin_pair<int, QSpinBox>(nullptr, camera_offset_x_spin, CAMERA_OFFSET_X_CONFIG, CAMERA_OFFSET_X_DEFAULT, camera->OffsetX.GetMin(), camera->OffsetX.GetMax(), 0);
    setup_slider_spin_pair<int, QSpinBox>(nullptr, camera_offset_y_spin, CAMERA_OFFSET_Y_CONFIG, CAMERA_OFFSET_Y_DEFAULT, camera->OffsetY.GetMin(), camera->OffsetY.GetMax(), 0);

    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_brightness_slider, camera_brightness_spin, CAMERA_BLACK_LEVEL_CONFIG, CAMERA_BLACK_LEVEL_DEFAULT, camera->BlackLevel.GetMin(), camera->BlackLevel.GetMax(), 0.1);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_gain_slider, camera_gain_spin, CAMERA_GAIN_CONFIG, CAMERA_GAIN_DEFAULT, camera->Gain.GetMin(), camera->Gain.GetMax(), 0.1);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_exposure_time_slider, camera_exposure_time_spin, CAMERA_EXPOSURE_TIME_CONFIG, CAMERA_EXPOSURE_TIME_DEFAULT, camera->ExposureTime.GetMin(), camera->ExposureTime.GetMax(), 0.1);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_balance_red_slider, camera_balance_red_spin, CAMERA_BALANCE_RED_CONFIG, CAMERA_BALANCE_RED_DEFAULT, camera->BalanceRatio.GetMin(), camera->BalanceRatio.GetMax(), 0.0);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_balance_blue_slider, camera_balance_blue_spin, CAMERA_BALANCE_BLUE_CONFIG, CAMERA_BALANCE_BLUE_DEFAULT, camera->BalanceRatio.GetMin(), camera->BalanceRatio.GetMax(), 0.0);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_frame_rate_slider, camera_frame_rate_spin, CAMERA_FRAME_RATE_CONFIG, CAMERA_FRAME_RATE_DEFAULT, camera->AcquisitionFrameRate.GetMin(), camera->AcquisitionFrameRate.GetMax(), 0.0);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_gamma_slider, camera_gamma_spin, CAMERA_GAMMA_CONFIG, CAMERA_GAMMA_DEFAULT, camera->Gamma.GetMin(), camera->Gamma.GetMax(), 0.0);
    setup_slider_spin_pair<double, QDoubleSpinBox>(camera_saturation_slider, camera_saturation_spin, CAMERA_SATURATION_CONFIG, CAMERA_SATURATION_DEFAULT, camera->Saturation.GetMin(), camera->Saturation.GetMax(), 0.0);
    // setup_slider_spin_pair<double, QDoubleSpinBox>(camera_sharpness_slider, camera_sharpness_spin, CAMERA_SHARPNESS_CONFIG, CAMERA_SHARPNESS_DEFAULT, camera->Sharpening.GetMin(), camera->Sharpening.GetMax(), 0.0);
}

template<typename T1, typename T2>
void CameraConfigurationDialog::setup_slider_spin_pair(QSlider * slider, T2 * spin, std::string configString, T1 defaultValue, T1 minValue, T1 maxValue, double maxOffset)
{
    if (spin)
    {
        QSettings & config = APP->config;

        spin->blockSignals(true);
        spin->setRange(minValue, maxValue - maxOffset);
        spin->setValue(std::is_same<T1, int>::value ? config.value(configString.c_str(), defaultValue).toInt() : config.value(configString.c_str(), defaultValue).toDouble());
        spin->blockSignals(false);

        if (slider)
        {
            slider->blockSignals(true);
            slider->setRange(0, 10000);
            slider->setValue(spin->value() * 10000.0 / spin->maximum());
            slider->blockSignals(false);
        }
    }
}

template<typename T>
void CameraConfigurationDialog::apply_slider_spin_pair(QSlider * slider, QDoubleSpinBox * spin, T i, std::string config)
{
    if (std::is_same<T, int>::value)
    {
        std::clamp((int)i, (int)slider->minimum(), (int)slider->maximum());
        double value = std::clamp(i * (spin->maximum() / (double)slider->maximum()), spin->minimum(), spin->maximum());

        APP->config.setValue(config.c_str(), value);
        _video_input.update_camera_parameters();

        spin->blockSignals(true);
        spin->setValue(APP->config.value(config.c_str()).toDouble());
        slider->setValue((double)slider->maximum() / spin->maximum() * APP->config.value(config.c_str()).toDouble());
        spin->blockSignals(false);
    }
    else if (std::is_same<T, double>::value)
    {
        std::clamp((double)i, (double)spin->minimum(), (double)spin->maximum());

        APP->config.setValue(config.c_str(), i);
        _video_input.update_camera_parameters();

        slider->blockSignals(true);
        spin->setValue(APP->config.value(config.c_str()).toDouble());
        slider->setValue((double)slider->maximum() / spin->maximum() * APP->config.value(config.c_str()).toDouble());
        slider->blockSignals(false);
    }
}

void CameraConfigurationDialog::on_camera_height_spin_valueChanged(int i)
{
    if (std::abs(i - camera_height_spin->minimum()) % 2 != 0) i++;
    std::clamp(i, camera_height_spin->minimum(), camera_height_spin->maximum());

    APP->config.setValue(CAMERA_HEIGHT_CONFIG, i);
    _video_input.update_camera_parameters();

    update_ui(false);
}

void CameraConfigurationDialog::on_camera_width_spin_valueChanged(int i)
{
    if (std::abs(i - camera_width_spin->minimum()) % 2 != 0) i++;
    std::clamp(i, camera_width_spin->minimum(), camera_width_spin->maximum());

    APP->config.setValue(CAMERA_WIDTH_CONFIG, i);
    _video_input.update_camera_parameters();

    update_ui(false);
}

void CameraConfigurationDialog::on_camera_offset_x_spin_valueChanged(int i)
{
    if (std::abs(i - camera_offset_x_spin->minimum()) % 2 != 0) i++;
    std::clamp(i, camera_offset_x_spin->minimum(), camera_offset_x_spin->maximum());

    APP->config.setValue(CAMERA_OFFSET_X_CONFIG, i);
    _video_input.update_camera_parameters();
}

void CameraConfigurationDialog::on_camera_offset_y_spin_valueChanged(int i)
{
    if (std::abs(i - camera_offset_y_spin->minimum()) % 2 != 0) i++;
    std::clamp(i, camera_offset_y_spin->minimum(), camera_offset_y_spin->maximum());

    APP->config.setValue(CAMERA_OFFSET_Y_CONFIG, i);
    _video_input.update_camera_parameters();
}

void CameraConfigurationDialog::on_camera_brightness_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_brightness_slider, camera_brightness_spin, i, CAMERA_BLACK_LEVEL_CONFIG);
}

void CameraConfigurationDialog::on_camera_brightness_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_brightness_slider, camera_brightness_spin, i, CAMERA_BLACK_LEVEL_CONFIG);
}

void CameraConfigurationDialog::on_camera_gain_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_gain_slider, camera_gain_spin, i, CAMERA_GAIN_CONFIG);
}

void CameraConfigurationDialog::on_camera_gain_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_gain_slider, camera_gain_spin, i, CAMERA_GAIN_CONFIG);
}

void CameraConfigurationDialog::on_camera_exposure_time_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_exposure_time_slider, camera_exposure_time_spin, i, CAMERA_EXPOSURE_TIME_CONFIG);
}

void CameraConfigurationDialog::on_camera_exposure_time_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_exposure_time_slider, camera_exposure_time_spin, i, CAMERA_EXPOSURE_TIME_CONFIG);
}

void CameraConfigurationDialog::on_camera_balance_red_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_balance_red_slider, camera_balance_red_spin, i, CAMERA_BALANCE_RED_CONFIG);
}

void CameraConfigurationDialog::on_camera_balance_red_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_balance_red_slider, camera_balance_red_spin, i, CAMERA_BALANCE_RED_CONFIG);
}

void CameraConfigurationDialog::on_camera_balance_blue_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_balance_blue_slider, camera_balance_blue_spin, i, CAMERA_BALANCE_BLUE_CONFIG);
}

void CameraConfigurationDialog::on_camera_balance_blue_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_balance_blue_slider, camera_balance_blue_spin, i, CAMERA_BALANCE_BLUE_CONFIG);
}

void CameraConfigurationDialog::on_camera_frame_rate_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_frame_rate_slider, camera_frame_rate_spin, i, CAMERA_FRAME_RATE_CONFIG);
}

void CameraConfigurationDialog::on_camera_frame_rate_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_frame_rate_slider, camera_frame_rate_spin, i, CAMERA_FRAME_RATE_CONFIG);
}

void CameraConfigurationDialog::on_camera_sharpness_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_sharpness_slider, camera_sharpness_spin, i, CAMERA_SHARPNESS_CONFIG);
}

void CameraConfigurationDialog::on_camera_sharpness_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_sharpness_slider, camera_sharpness_spin, i, CAMERA_SHARPNESS_CONFIG);
}

void CameraConfigurationDialog::on_camera_hue_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_hue_slider, camera_hue_spin, i, CAMERA_HUE_CONFIG);
}

void CameraConfigurationDialog::on_camera_hue_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_hue_slider, camera_hue_spin, i, CAMERA_HUE_CONFIG);
}

void CameraConfigurationDialog::on_camera_saturation_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_saturation_slider, camera_saturation_spin, i, CAMERA_SATURATION_CONFIG);
}

void CameraConfigurationDialog::on_camera_saturation_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_saturation_slider, camera_saturation_spin, i, CAMERA_SATURATION_CONFIG);
}

void CameraConfigurationDialog::on_camera_gamma_slider_valueChanged(int i)
{
    apply_slider_spin_pair<int>(camera_gamma_slider, camera_gamma_spin, i, CAMERA_GAMMA_CONFIG);
}

void CameraConfigurationDialog::on_camera_gamma_spin_valueChanged(double i)
{
    apply_slider_spin_pair<double>(camera_gamma_slider, camera_gamma_spin, i, CAMERA_GAMMA_CONFIG);
}