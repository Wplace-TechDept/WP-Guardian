#pragma once
#include <opencv2/opencv.hpp>
#include <functional>
#include "Settings/Core/Settings.h"

struct Base64ImageCache {
    std::string data = "";  // Base64文字列
    bool hasData = false;   // キャッシュが有効か
};

struct AppState {
    // 画像・差分情報
    bool update = false;
    int totalPixels = 0;
    int diffPixels = 0;
    double diffPercent = 0.0;
    int width = 0;
    int height = 0;

    cv::Mat originalImg;
    cv::Mat realtimeImg;
    cv::Mat diffImg;

	Base64ImageCache originalBase64;
	Base64ImageCache realtimeBase64;
	Base64ImageCache diffBase64;

    // 設定へのポインタ
    Settings::AppSettings* appSettings = nullptr;
    Settings::WindowStates* windowStates = nullptr;

    // settingsWindow へのアクセサ
    std::function<Settings::AppSettings()> getTempSettings;
    std::function<void(const Settings::AppSettings&)> setTempSettings;
};