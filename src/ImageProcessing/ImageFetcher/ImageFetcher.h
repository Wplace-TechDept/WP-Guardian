#pragma once
#include <cpr/cpr.h>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <string>

class ImageFetcher {
public:
    cv::Mat FetchTilesAndCrop(
        int tile_x, int tile_y,
        int x_in_tile, int y_in_tile,
        int ref_width, int ref_height,
        int TILE_SIZE = 1000
    );
    void Abort() {abort_fetch = true;}
    void Reset() {abort_fetch = false;}
private:
    std::atomic<bool> abort_fetch = false;
    cpr::Response CancellableFetch(const std::string& url, const cpr::Header& headers);
};