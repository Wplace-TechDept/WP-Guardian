#define NOMINMAX

#include "ImageProcessing.h"
#include <opencv2/opencv.hpp>
#include <algorithm>

ImageProcessing::ImageProcessing(){
}
ImageProcessing::~ImageProcessing() {
    stop();
}

void ImageProcessing::SetTileParams(int tileX, int tileY, int pixelX, int pixelY) {
	fetcher.Abort();
    std::scoped_lock lock(settingsMutex);
    this->tileX = tileX;
    this->tileY = tileY;
    this->pixelX = pixelX;
    this->pixelY = pixelY;
    fetcher.Reset();
}
void ImageProcessing::SetDiffPattern(const DiffPattern& pattern) {
    std::scoped_lock lock(settingsMutex);
    this->diffPattern = pattern;
}
void ImageProcessing::SetUpdateSpeed(double speed) {
    std::scoped_lock lock(settingsMutex);
    this->speed = speed;
}
void ImageProcessing::SetOriginalImage(const cv::Mat& img) {
    int w, h;
    fetcher.Abort();
    {
        std::scoped_lock lock(settingsMutex);
        this->originalImg = img.clone();
        this->width = img.cols;
        this->height = img.rows;
		w = this->width;
		h = this->height;
    }
    {
        std::scoped_lock lock(outputMutex);
        realtimeImg = cv::Mat::zeros(h, w, CV_8UC4);
		diffImg = cv::Mat::zeros(h, w, CV_8UC4);
        diffPercent = 0.0;
        totalOpaquePixels = 0;
        diffPixels = 0;
        newFrameReady = false;
    }
    fetcher.Reset();
}
bool ImageProcessing::fetchOutput(Output& out) {
	std::scoped_lock lock(outputMutex);
    if (!newFrameReady) {
        return false;
    }
    out.realtimeImg = this->realtimeImg.clone();
    out.diffImg = this->diffImg.clone();
    out.diffPercent = this->diffPercent;
    out.totalOpaquePixels = this->totalOpaquePixels;
    out.diffPixels = this->diffPixels;
    newFrameReady = false;
	return true;
}

void ImageProcessing::start() {
    stopThread = false;
    fetcher.Reset();

    worker = std::thread([this]()
        {
            while (!stopThread) {
                int localTile_x, localTile_y, localPixel_x, localPixel_y, localWidth, localHeight;
                DiffPattern localDiffPattern;
                double localSpeed;
                cv::Mat localOriginalImg;
                {
                    std::scoped_lock lock(settingsMutex);
                    localTile_x = tileX;
                    localTile_y = tileY;
                    localPixel_x = pixelX;
                    localPixel_y = pixelY;
                    localWidth = width;
                    localHeight = height;
                    localDiffPattern = diffPattern;
                    localSpeed = speed;
                    localOriginalImg = originalImg.clone();
                }
				cv::Mat newImg = fetcher.FetchTilesAndCrop(localTile_x, localTile_y, localPixel_x, localPixel_y, localWidth, localHeight);
                if (stopThread || newImg.empty()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
                }
				newImg = ApplyAlphaMask(newImg, localOriginalImg);
                auto [diff, totalOpaquePx, diffPx] = imageDifferenceSafe(localOriginalImg, newImg, localDiffPattern);
                double percent = (totalOpaquePx > 0) ? (double)diffPx / totalOpaquePx * 100.0 : 0.0;
                {
					std::scoped_lock lock(outputMutex);
                    realtimeImg = newImg.clone();
                    diffImg = diff.clone();
                    diffPercent = percent;
                    totalOpaquePixels = totalOpaquePx;
                    diffPixels = diffPx;
					newFrameReady = true;
					cv_newFrame.notify_one();
                }
                for (int i = 0; i < 10 && !stopThread; i++) std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(localSpeed * 100)));
            }
	    });
}

void ImageProcessing::stop() {
    stopThread = true;
    fetcher.Abort();
    if (worker.joinable()) {
        worker.join();
    }
}

std::tuple<cv::Mat, int, int> ImageProcessing::imageDifferenceSafe(
    const cv::Mat& img1,
    const cv::Mat& img2,
    const DiffPattern& pattern)
{
    if (img1.empty() || img2.empty())
        return { cv::Mat(), 0, 0 };

    int w = std::min(img1.cols, img2.cols);
    int h = std::min(img1.rows, img2.rows);
    if (w <= 0 || h <= 0)
        return { cv::Mat(), 0, 0 };

    cv::Mat diffImage = cv::Mat::zeros(img1.size(), CV_8UC4);
    cv::Rect roi(0, 0, w, h);
    cv::Mat roi1 = img1(roi);
    cv::Mat roi2 = img2(roi);

    cv::Mat alpha;
    if (roi1.channels() == 4)
        cv::extractChannel(roi1, alpha, 3);
    else
        alpha = cv::Mat::ones(roi.size(), CV_8U) * 255;

    cv::Mat merged = cv::Mat::zeros(roi.size(), CV_8UC4);
    int changedPixels = 0;

    for (int y = 0; y < h; ++y)
    {
        const cv::Vec4b* p1 = roi1.ptr<cv::Vec4b>(y);
        const cv::Vec4b* p2 = roi2.ptr<cv::Vec4b>(y);
        const uchar* maskPtr = alpha.ptr<uchar>(y);
        cv::Vec4b* dst = merged.ptr<cv::Vec4b>(y);

        for (int x = 0; x < w; ++x)
        {
            if (!maskPtr[x])
                continue;

            bool changed = (p1[x] != p2[x]);
            if (changed)
                changedPixels++;

            switch (pattern.type)
            {
            case DiffPatternType::AbsDiff:
                for (int c = 0; c < 3; ++c)
                    dst[x][c] = std::abs(p1[x][c] - p2[x][c]);
                dst[x][3] = 255;
                break;

            case DiffPatternType::ColorMask:
                dst[x] = changed ? cv::Vec4b(
                    (uchar)(pattern.maskColor.b * 255),
                    (uchar)(pattern.maskColor.g * 255),
                    (uchar)(pattern.maskColor.r * 255),
                    (uchar)(pattern.maskColor.a * 255))
                    : cv::Vec4b(0, 0, 0, 255);
                break;

            case DiffPatternType::OverlayTemplate:
                if (changed)
                    dst[x] = cv::Vec4b(
                        (uchar)(pattern.maskColor.b * 255),
                        (uchar)(pattern.maskColor.g * 255),
                        (uchar)(pattern.maskColor.r * 255),
                        (uchar)(pattern.maskColor.a * 255));
                else
                    dst[x] = p1[x];
                break;
            }
        }
    }

    merged.copyTo(diffImage(roi));
    int totalOpaquePixels = cv::countNonZero(alpha);
    return { diffImage, totalOpaquePixels, changedPixels };
}
cv::Mat ImageProcessing::ApplyAlphaMask(const cv::Mat& src, const cv::Mat& mask)
{
    if (src.empty() || mask.empty())
        return cv::Mat();
    CV_Assert(src.size() == mask.size());
    CV_Assert(src.type() == CV_8UC4 && mask.type() == CV_8UC4);

    cv::Mat dst = src.clone();
    for (int y = 0; y < src.rows; ++y)
    {
        const cv::Vec4b* srcRow = src.ptr<cv::Vec4b>(y);
        const cv::Vec4b* maskRow = mask.ptr<cv::Vec4b>(y);
        cv::Vec4b* dstRow = dst.ptr<cv::Vec4b>(y);
        for (int x = 0; x < src.cols; ++x)
        {
            uchar srcA = srcRow[x][3] ? 255 : 0;
            uchar maskA = maskRow[x][3] ? 255 : 0;
            dstRow[x][3] = srcA & maskA;
        }
    }
    return dst;
}
