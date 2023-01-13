/*
Copyright (c) 2012, Daniel Moreno and Gabriel Taubin
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

#include "CalibrationData.hpp"

#include <QFileInfo>

#include <iostream>
#include <opencv2/calib3d.hpp>

CalibrationData::CalibrationData() :
    cam_K(), cam_kc(),
    proj_K(), proj_kc(),
    R(), T(), E(), F(),
    // H1(), H2(),
    cam_error(0.0), proj_error(0.0), stereo_error(0.0),
    filename()
{
}

CalibrationData::~CalibrationData()
{
}

void CalibrationData::clear(void)
{
    cam_K = cv::Mat();
    cam_kc = cv::Mat();
    proj_K = cv::Mat();
    proj_kc = cv::Mat();
    R = cv::Mat();
    T = cv::Mat();
    E = cv::Mat();
    F = cv::Mat();
    // H1 = cv::Mat();
    // H2 = cv::Mat();
    filename = QString();

    cam_error = 0.0;
    proj_error = 0.0;
    stereo_error = 0.0;

    cam_width = 0;
    cam_height = 0;
    proj_width = 0;
    proj_height = 0;

    cam_per_view_errors = cv::Mat();
    proj_per_view_errors = cv::Mat();
    stereo_per_view_errors = cv::Mat();
}

bool CalibrationData::is_valid(void) const
{
    return (cam_K.data && cam_kc.data && proj_K.data && proj_kc.data && R.data && T.data && E.data && F.data/* && H1.data && H2.data*/);
}

bool CalibrationData::load_calibration(QString const& filename)
{
    QFileInfo info(filename);
    QString type = info.suffix();

    if (type=="yml") {return load_calibration_yml(filename);}

    return false;
}

bool CalibrationData::save_calibration(QString const& filename)
{
    QFileInfo info(filename);
    QString type = info.suffix();

    if (type=="yml") {return save_calibration_yml(filename);}
    if (type=="m"  ) {return save_calibration_matlab(filename);}
    if (type=="json") { return save_calibration_json(filename);}

    return false;
}

bool CalibrationData::load_calibration_yml(QString const& filename)
{
    cv::FileStorage fs(filename.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        return false;
    }

    fs["cam_K"] >> cam_K;
    fs["cam_kc"] >> cam_kc;
    fs["proj_K"] >> proj_K;
    fs["proj_kc"] >> proj_kc;
    fs["R"] >> R;
    fs["T"] >> T;
    fs["E"] >> E;
    fs["F"] >> F;
    // fs["H1"] >> H1;
    // fs["H2"] >> H2;

    fs["cam_error"] >> cam_error;
    fs["proj_error"] >> proj_error;
    fs["stereo_error"] >> stereo_error;

    fs.release();

    this->filename = filename;

    return true;
}

bool CalibrationData::save_calibration_yml(QString const& filename)
{
    cv::FileStorage fs(filename.toStdString(), cv::FileStorage::WRITE);
    if (!fs.isOpened())
    {
        return false;
    }

    fs << "cam_K" << cam_K << "cam_kc" << cam_kc
       << "proj_K" << proj_K << "proj_kc" << proj_kc
       << "R" << R << "T" << T << "E" << E << "F" << F
    //    << "H1" << H1 << "H2" << H2
       << "cam_error" << cam_error
       << "proj_error" << proj_error
       << "stereo_error" << stereo_error
       ;
    fs.release();

    this->filename = filename;

    return true;
}

bool CalibrationData::save_calibration_json(QString const& filename)
{
    FILE* fp = fopen(qPrintable(filename), "w");
    if (!fp)
    {
        return false;
    }

    cv::Mat rvec;
    cv::Rodrigues(R, rvec);
    fprintf(fp,
            "{\n"
                "\t\"cam_int\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 3,\n"
                    "\t\t\"cols\" : 3,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n"
                "\t},\n"
                "\t\"cam_dist\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 1,\n"
                    "\t\t\"cols\" : 5,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf, %lf, %lf, %lf]\n"
                "\t},\n"
                "\t\"proj_int\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 3,\n"
                    "\t\t\"cols\" : 3,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n"
                "\t},\n"
                "\t\"proj_dist\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 1,\n"
                    "\t\t\"cols\" : 5,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf, %lf, %lf, %lf]\n"
                "\t},\n"
                "\t\"rotation\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 3,\n"
                    "\t\t\"cols\" : 3,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n"
                "\t},\n"
                "\t\"translation\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 3,\n"
                    "\t\t\"cols\" : 1,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf, %lf]\n"
                "\t},\n"
                "\t\"cam_shape\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 2,\n"
                    "\t\t\"cols\" : 1,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf]\n"
                "\t},\n"
                "\t\"proj_shape\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 2,\n"
                    "\t\t\"cols\" : 1,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf, %lf]\n"
                "\t},\n"
                "\t\"cam_error\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 1,\n"
                    "\t\t\"cols\" : 1,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf]\n"
                "\t},\n"
                "\t\"proj_error\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 1,\n"
                    "\t\t\"cols\" : 1,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf]\n"
                "\t},\n"
                "\t\"stereo_error\": {\n"
                    "\t\t\"type_id\": \"opencv-matrix\",\n"
                    "\t\t\"rows\" : 1,\n"
                    "\t\t\"cols\" : 1,\n"
                    "\t\t\"dt\" : \"d\",\n"
                    "\t\t\"data\" : [%lf]\n"
                "\t}\n"
            "}\n",
        cam_K.at<double>(0, 0), cam_K.at<double>(0, 1), cam_K.at<double>(0, 2), cam_K.at<double>(1, 0), cam_K.at<double>(1, 1), cam_K.at<double>(1, 2), cam_K.at<double>(2, 0), cam_K.at<double>(2, 1), cam_K.at<double>(2, 2),
        cam_kc.at<double>(0, 0), cam_kc.at<double>(0, 1), cam_kc.at<double>(0, 2), cam_kc.at<double>(0, 3), cam_kc.at<double>(0, 4),
        proj_K.at<double>(0, 0), proj_K.at<double>(0, 1), proj_K.at<double>(0, 2), proj_K.at<double>(1, 0), proj_K.at<double>(1, 1), proj_K.at<double>(1, 2), proj_K.at<double>(2, 0), proj_K.at<double>(2, 1), proj_K.at<double>(2, 2),
        proj_kc.at<double>(0, 0), proj_kc.at<double>(0, 1), proj_kc.at<double>(0, 2), proj_kc.at<double>(0, 3), proj_kc.at<double>(0, 4),
        R.at<double>(0, 0), R.at<double>(0, 1), R.at<double>(0, 2), R.at<double>(1, 0), R.at<double>(1, 1), R.at<double>(1, 2), R.at<double>(2, 0), R.at<double>(2, 1), R.at<double>(2, 2),
        // Translation is divided by 1000.0f in order to convert from mm to m. Assumes that the input unit for checkerboard size is in mm.
        T.at<double>(0, 0) / 1000.0f, T.at<double>(1, 0) / 1000.0f, T.at<double>(2, 0) / 1000.0f,
        cam_height, cam_width,
        proj_height, proj_width,
        cam_error, proj_error, stereo_error
    );
    fclose(fp);

    return true;
}

bool CalibrationData::save_calibration_matlab(QString const& filename)
{
    FILE * fp = fopen(qPrintable(filename), "w");
    if (!fp)
    {
        return false;
    }

    cv::Mat rvec;
    cv::Rodrigues(R, rvec);
    fprintf(fp, 
        "%% Projector-Camera Stereo calibration parameters:\n"
        "\n"
        "%% Intrinsic parameters of camera:\n"
        "fc_left = [ %lf %lf ]; %% Focal Length\n"
        "cc_left = [ %lf %lf ]; %% Principal point\n"
        "alpha_c_left = [ %lf ]; %% Skew\n"
        "kc_left = [ %lf %lf %lf %lf %lf ]; %% Distortion\n"
        "\n"
        "%% Intrinsic parameters of projector:\n"
        "fc_right = [ %lf %lf ]; %% Focal Length\n"
        "cc_right = [ %lf %lf ]; %% Principal point\n"
        "alpha_c_right = [ %lf ]; %% Skew\n"
        "kc_right = [ %lf %lf %lf %lf %lf ]; %% Distortion\n"
        "\n"
        "%% Extrinsic parameters (position of projector wrt camera):\n"
        "om = [ %lf %lf %lf ]; %% Rotation vector\n"
        "T = [ %lf %lf %lf ]; %% Translation vector\n",
        cam_K.at<double>(0,0), cam_K.at<double>(1,1), cam_K.at<double>(0,2), cam_K.at<double>(1,2), cam_K.at<double>(0,1),
        cam_kc.at<double>(0,0), cam_kc.at<double>(0,1), cam_kc.at<double>(0,2), cam_kc.at<double>(0,3), cam_kc.at<double>(0,4), 
        proj_K.at<double>(0,0), proj_K.at<double>(1,1), proj_K.at<double>(0,2), proj_K.at<double>(1,2), proj_K.at<double>(0,1),
        proj_kc.at<double>(0,0), proj_kc.at<double>(0,1), proj_kc.at<double>(0,2), proj_kc.at<double>(0,3), proj_kc.at<double>(0,4),
        rvec.at<double>(0,0), rvec.at<double>(1,0), rvec.at<double>(2,0), 
        T.at<double>(0,0), T.at<double>(1,0), T.at<double>(2,0)
        );
    fclose(fp);

    return true;
}

void CalibrationData::display(std::ostream & stream) const
{
    stream << "Camera Calib: " << std::endl
        << " - reprojection error: " << cam_error << std::endl
        << " - K:\n" << cam_K << std::endl
        << " - kc: " << cam_kc << std::endl
        ;

    stream << std::endl;
    stream << "Projector Calib: " << std::endl
        << " - reprojection error: " << proj_error << std::endl
        << " - K:\n" << proj_K << std::endl
        << " - kc: " << proj_kc << std::endl
        ;

    // Log the per-view errors (per-view error dim: n x 1)
    stream << " - per-view reprojection errors: [" << "\n";
    for (int r = 0; r < proj_per_view_errors.rows; ++r)
        stream << r << ": " << proj_per_view_errors.row(r) << '\n';
    stream << "]" << std::endl;

    stream << std::endl;
    stream << "Stereo Calib: " << std::endl
        << " - reprojection error: " << stereo_error << std::endl
        << " - R:\n" << R << std::endl
        << " - T:\n" << T << std::endl
        ;

    // Log the per-view errors (per-view error dim: n x 2)
    stream << " - per-view reprojection errors: [" << "\n";
    for (int r = 0; r < stereo_per_view_errors.rows; ++r)
        stream << r << ": " << stereo_per_view_errors.row(r) << '\n';
    stream << "]" << std::endl;
}

