#define NOMINMAX

#include "ImageProcessing.h"
#include <cpr/cpr.h>
#include <opencv2/opencv.hpp>
#include <algorithm>

ImageProcessing::ImageProcessing(){
}
ImageProcessing::~ImageProcessing() {
    stop();
}

void ImageProcessing::SetTileParams(int tileX, int tileY, int pixelX, int pixelY) {
	this->abort_fetch = true;
    std::scoped_lock lock(settingsMutex);
    this->tileX = tileX;
    this->tileY = tileY;
    this->pixelX = pixelX;
    this->pixelY = pixelY;
	this->abort_fetch = false;
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
	this->abort_fetch = true;
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
	this->abort_fetch = false;
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
    abort_fetch = false;

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
				cv::Mat newImg = fetch_tiles_and_crop(localTile_x, localTile_y, localPixel_x, localPixel_y, localWidth, localHeight);
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
    abort_fetch = true;
    if (worker.joinable()) {
        worker.join();
    }
}

cpr::Response ImageProcessing::cancellable_fetch(const std::string& url, const cpr::Header& headers)
{
    try
    {
        auto future = cpr::GetAsync(cpr::Url{ url }, headers, cpr::Timeout{ 5000 });

        while (!abort_fetch && future.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready)
        {
        }

        if (abort_fetch)
        {
            try
            {
                //  ߂ l  ϐ  Ɋi [   Ĕj  
                cpr::Response ignored = future.get();
                (void)ignored; //      I ɔj  
            }
            catch (...)
            {
            }
            return cpr::Response{};
        }

        return future.get();
    }
    catch (...)
    {
        return cpr::Response{};
    }
}

cv::Mat ImageProcessing::fetch_tiles_and_crop(
    int tile_x, int tile_y, int x_in_tile, int y_in_tile,
    int ref_width, int ref_height, int TILE_SIZE)
{
    int end_x = x_in_tile + ref_width;
    int end_y = y_in_tile + ref_height;
    int tile_x_end = tile_x + (end_x / TILE_SIZE);
    int tile_y_end = tile_y + (end_y / TILE_SIZE);

    int canvas_w = (tile_x_end - tile_x + 1) * TILE_SIZE;
    int canvas_h = (tile_y_end - tile_y + 1) * TILE_SIZE;
    cv::Mat canvas(canvas_h, canvas_w, CV_8UC4, cv::Scalar(0, 0, 0, 0));

    cpr::Header headers = {
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"},
        {"Accept", "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8"},
        {"Referer", "https://www.google.com/"} };
    
    int tile_index = 0;
    for (int ty = tile_y; ty <= tile_y_end; ++ty)
    {
        for (int tx = tile_x; tx <= tile_x_end; ++tx)
        {
            if (abort_fetch)
                return cv::Mat();
            auto now = std::chrono::system_clock::now();
            auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            std::string url = "https://backend.wplace.live/files/s0/tiles/" + std::to_string(tx) + "/" + std::to_string(ty) + ".png" + "?t=" + std::to_string(epoch);
            cpr::Response r = ImageProcessing::cancellable_fetch(url, headers);
            if (abort_fetch)
                return cv::Mat();
            if (r.status_code != 200 || r.text.empty())
                return cv::Mat();

            std::vector<uchar> data(r.text.begin(), r.text.end());
            cv::Mat img = cv::imdecode(data, cv::IMREAD_UNCHANGED);
            if (img.empty())
                return cv::Mat();

            int ty_rel = tile_index / (tile_x_end - tile_x + 1);
            int tx_rel = tile_index % (tile_x_end - tile_x + 1);
            int oy = ty_rel * TILE_SIZE;
            int ox = tx_rel * TILE_SIZE;

            cv::Rect roi_dst(ox, oy, img.cols, img.rows);
            cv::Rect canvas_rect(0, 0, canvas.cols, canvas.rows);
            roi_dst &= canvas_rect;

            if (roi_dst.width > 0 && roi_dst.height > 0)
            {
                cv::Rect roi_src(0, 0, roi_dst.width, roi_dst.height);
                img(roi_src).copyTo(canvas(roi_dst));
            }
            tile_index++;
        }
    }

    cv::Rect roi(x_in_tile, y_in_tile, ref_width, ref_height);
    cv::Rect canvas_rect(0, 0, canvas.cols, canvas.rows);
    roi &= canvas_rect;
    if (roi.width <= 0 || roi.height <= 0)
        return cv::Mat();
    return canvas(roi).clone();
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

    //  A   t @ `     l  
    cv::Mat alpha;
    if (roi1.channels() == 4)
        cv::extractChannel(roi1, alpha, 3);
    else
        alpha = cv::Mat::ones(roi.size(), CV_8U) * 255;

    //      v Z
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
