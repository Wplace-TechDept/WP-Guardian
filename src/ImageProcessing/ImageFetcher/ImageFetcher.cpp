#include "ImageFetcher.h"
#include <chrono>
#include <opencv2/imgcodecs.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#endif

#ifndef __EMSCRIPTEN__
cpr::Response ImageFetcher::CancellableFetch(const std::string& url, const cpr::Header& headers)
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
                cpr::Response ignored = future.get();
                (void)ignored;
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
#endif

cv::Mat ImageFetcher::FetchTilesAndCrop(
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
#ifndef __EMSCRIPTEN__
    cpr::Header headers = {
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"},
        {"Accept", "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8"},
        {"Referer", "https://www.google.com/"} };
#endif
    int tile_index = 0;
    for (int ty = tile_y; ty <= tile_y_end; ++ty)
    {
        for (int tx = tile_x; tx <= tile_x_end; ++tx)
        {
            if (abort_fetch)
                return cv::Mat();
            auto now = std::chrono::system_clock::now();
            auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            std::string url = "https://backend.wplace.live/tile/" + std::to_string(tx) + "/" + std::to_string(ty) + ".png" + "?t=" + std::to_string(epoch);
#ifndef __EMSCRIPTEN__
            cpr::Response r = ImageFetcher::CancellableFetch(url, headers);
            if (abort_fetch)
                return cv::Mat();
            if (r.status_code != 200 || r.text.empty())
                return cv::Mat();

            std::vector<uchar> data(r.text.begin(), r.text.end());
#else

            emscripten_fetch_attr_t attr;
            emscripten_fetch_attr_init(&attr);
            strcpy(attr.requestMethod, "GET");
            attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;

            emscripten_fetch_t* fetch = emscripten_fetch(&attr, url.c_str());
            if (abort_fetch || fetch->status != 200 || fetch->numBytes == 0) {
                emscripten_fetch_close(fetch);
                return cv::Mat();
            }
            std::vector<uchar> data(fetch->data, fetch->data + fetch->numBytes);
            emscripten_fetch_close(fetch);
#endif
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