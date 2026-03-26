#pragma once
#include <opencv2/opencv.hpp>
#include <atomic>
#include <string>
#ifndef __EMSCRIPTEN__
#include <cpr/cpr.h>
#endif

class ImageFetcher {
public:
    ~ImageFetcher() { abort_fetch = true; }
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

#ifndef __EMSCRIPTEN__
    cpr::Response CancellableFetch(const std::string& url, const cpr::Header& headers);
#endif
};