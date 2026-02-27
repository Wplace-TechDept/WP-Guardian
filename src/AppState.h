#pragma once
#include <opencv2/opencv.hpp>
#include "Settings/Core/Settings.h"

struct AppState {
    // wsVar の getter/setter でアクセスされるもの
    bool update = false;
    int totalPixels = 0;
    int diffPixels = 0;
    double diffPercent = 0.0;
    int width = 0;
    int height = 0;

    cv::Mat originalImg;
    cv::Mat realtimeImg;
    cv::Mat diffImg;

    // 設定へのポインタ（実体はmainが持つ）
    Settings::AppSettings* appSettings = nullptr;
    Settings::WindowStates* windowStates = nullptr;
};