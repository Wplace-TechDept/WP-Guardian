#pragma once
#include "DiffPattern.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include <cpr/cpr.h>

class ImageProcessing {
public:
    ImageProcessing();
    ~ImageProcessing();


    void SetTileParams(int tileX, int tileY, int pixelX, int pixelY);
    void SetDiffPattern(const DiffPattern& pattern);
    void SetUpdateSpeed(double speed);
    void SetOriginalImage(const cv::Mat& img);

    struct Output {
        cv::Mat realtimeImg;
        cv::Mat diffImg;
        double diffPercent;
        int totalOpaquePixels;
        int diffPixels;
    };

    void start();
    void stop();

    bool fetchOutput(Output& out);
private:
    std::thread worker;
    std::atomic<bool> stopThread{ false };
    std::atomic<bool> abort_fetch{ false };

    // 設定系(input)
    std::mutex settingsMutex;
    int tileX, tileY, pixelX, pixelY;
    DiffPattern diffPattern;
    double speed;
    cv::Mat originalImg;

    int width, height;

    // 出力系(output)
    std::mutex outputMutex;
    cv::Mat realtimeImg,diffImg;
    int totalOpaquePixels, diffPixels;
    double diffPercent;
    std::condition_variable cv_newFrame;
    bool newFrameReady = false;


private:
    cpr::Response cancellable_fetch(const std::string& url, const cpr::Header& headers);
    cv::Mat fetch_tiles_and_crop(
        int tile_x, int tile_y, int pixel_x, int pixel_y,
        int ref_width, int ref_height, int TILE_SIZE = 1000);
    std::tuple<cv::Mat, int, int> imageDifferenceSafe(
        const cv::Mat& img1,
        const cv::Mat& img2,
        const DiffPattern& pattern);
    cv::Mat ApplyAlphaMask(const cv::Mat& src, const cv::Mat& mask);


};
