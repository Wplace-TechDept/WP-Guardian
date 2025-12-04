#define NOMINMAX // min/maxマクロの競合を回避

#include "ImWS.h"
#include "Base64.h"
#include "JsonConverters.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <opencv2/opencv.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <cpr/cpr.h>
#include <vector>
#include <algorithm> // std::min/std::max
#include <tuple>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <fstream> // C++のファイルストリームを使用
#include <nlohmann/json.hpp>
#include <fcntl.h>   // ← _O_U8TEXT などの定義
#include <io.h>      // ← _setmode の定義

// WebSocketサーバーのインスタンス
server wsServer;
std::atomic<bool> running{ true };

using json = nlohmann::json;
// アプリバージョン

const std::string APP_VERSION = "v2.1.0"; //バージョン更新時にかならず書き換える


// --- 設定などグローバル変数 ---
static bool showOriginal = true;
static bool showRealtime = true;
static bool showDiff = true;
static bool showSettings = true;
static bool showInfo = true;
static bool showWindowManager = true;
static bool showModuleSeting = true;

static int tile_x = 1818;
static int tile_y = 806;
static int pixel_x = 989;
static int pixel_y = 358;

static float UpdateSpeed = 4.0f;

static bool isLinkZoom = true;

static DiffPattern diffPattern(DiffPatternType::ColorMask);

static ImVec4 MaskColor = ImVec4(0, 1, 0, 1);

static std::string szFile = "assets\\template.png";
char szFileBuffer[MAX_PATH] = { 0 };
std::string szFileStr;

static bool autoStartModules = false;
static bool useServer = true;
static std::string serverIP = "127.0.0.1";  // すべてのネットワークインターフェイスで受け付ける
static unsigned short serverPort = 9002;

static bool foundUpdate = false;
static std::string latestVersion = "";
static std::string updateURL = "";
static bool showUpdateMessage = true;
// ----------------------------

struct Version {
    int major, minor, patch;
};

Version parseVersion(const std::string& v) {
    std::string s = (v[0] == 'v') ? v.substr(1) : v;

    Version ver{};
    ver.major = ver.minor = ver.patch = 0;

    int count = sscanf(s.c_str(), "%d.%d.%d", &ver.major, &ver.minor, &ver.patch);
    return ver;
}

bool isNewer(const Version& remote, const Version& local) {
    if (remote.major != local.major) return remote.major > local.major;
    if (remote.minor != local.minor) return remote.minor > local.minor;
    return remote.patch > local.patch;
}

std::string getLatestVersion(const std::string& owner, const std::string& repo) {
    std::string url = "https://api.github.com/repos/" + owner + "/" + repo + "/releases/latest";

    cpr::Response r = cpr::Get(
        cpr::Url{ url },
        cpr::Header{ {"User-Agent", "MyApp"} }
    );

    if (r.status_code != 200) {
        std::cerr << "Failed: " << r.status_code << "\n";
        return "";
    }

    auto json = nlohmann::json::parse(r.text);
    if (!json.contains("tag_name")) return "";

    return json["tag_name"];
}

void checkUpdate() {
    if (!foundUpdate) {
        std::string latest = getLatestVersion("Kikumon-Tech", "WP-Guardian");
        if (latest.empty()) {
            std::cout << "最新バージョン取得失敗" << std::endl;
            return;
        }

        Version local = parseVersion(APP_VERSION);
        Version remote = parseVersion(latest);

        if (isNewer(remote, local)) {
            std::cout << "アップデートがあります！ 最新: " << latest << std::endl;
			latestVersion = latest;
			updateURL = "https://github.com/Kikumon-Tech/WP-Guardian/releases/tag/" + latest;
            foundUpdate = true;
        }
        else {
            std::cout << "最新です" << std::endl;
            foundUpdate = false;
            showUpdateMessage = true;
        }
    }
    
}
bool showVersion = foundUpdate && showUpdateMessage;

void showVersionWindow() {
    if (!showVersion) return;
    if (!ImGui::Begin("バージョン", &showVersion)) {
        ImGui::End();
        return;
    }
    static bool loadedUpdate = false;
    if (!loadedUpdate) {
        checkUpdate();
        loadedUpdate = true;
    }
    if (foundUpdate) {
        ImGui::Text("新しいバージョンがあります！ 最新バージョン: %s", latestVersion.c_str());
    }
    else {
        ImGui::Text("既に最新バージョンです！");
    }
    if (ImGui::Button("アップデートページを開く")) {
        ShellExecuteA(NULL, "open", updateURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    // 内部フラグは hideUpdateMessage（trueなら非表示）
    bool hideUpdateMessage = !showUpdateMessage;

    // UI 側で反転させて渡す
    if (ImGui::Checkbox("このメッセージを再度表示しない", &hideUpdateMessage)) {
        showUpdateMessage = !hideUpdateMessage; // 内部フラグに反映
    }

    ImGui::End();
    if (!showVersion) {
        loadedUpdate = false;
    }
}


void SetupConsoleIfDebug()
{
#ifdef _DEBUG
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    // UTF-8 コードページに変更
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);


    std::cout << u8"[デバッグ] コンソールを開きました！" << std::endl;
#endif
}

// --- モジュールJson用グローバル変数 ---
bool showModuleManager = false;  // ウィンドウ表示フラグ
std::string moduleListPath = "assets/moduleList.json";
json moduleList;

// --- JSON読み込み ---
bool LoadModuleList()
{
    std::ifstream file(moduleListPath);
    if (!file.is_open()) {
        std::cerr << "moduleList.json が開けません: " << moduleListPath << std::endl;
        return false;
    }
    file >> moduleList;
    file.close();
    return true;
}

// --- JSON保存 ---
bool SaveModuleList()
{
    std::ofstream file(moduleListPath);
    if (!file.is_open()) {
        std::cerr << "moduleList.json の保存に失敗: " << moduleListPath << std::endl;
        return false;
    }
    file << moduleList.dump(4);
    file.close();
    return true;
}

// --- ウィンドウ描画 ---
void ShowModuleManagerWindow()
{
    if (!showModuleManager) return;

    if (!ImGui::Begin("モジュール管理", &showModuleManager))
    {
        ImGui::End();
        return;
    }

    static bool loaded = false;
    if (!loaded)
    {
        if (!LoadModuleList())
            moduleList["modules"] = json::array();
        loaded = true;
    }

    if (moduleList.contains("modules") && moduleList["modules"].is_array())
    {
        // 削除用に保持しておくインデックス
        int deleteIndex = -1;

        for (size_t i = 0; i < moduleList["modules"].size(); ++i)
        {
            auto& module = moduleList["modules"][i];

            // enabled チェックボックス
            bool enabled = module.value("enabled", true);
            if (ImGui::Checkbox(("有効##" + std::to_string(i)).c_str(), &enabled))
                module["enabled"] = enabled;

            // 名前編集
            char nameBuf[128];
            strncpy(nameBuf, module.value("name", "").c_str(), sizeof(nameBuf));
            if (ImGui::InputText(("名前##" + std::to_string(i)).c_str(), nameBuf, sizeof(nameBuf)))
                module["name"] = std::string(nameBuf);

            // コマンド編集
            char cmdBuf[256];
            strncpy(cmdBuf, module.value("command", "").c_str(), sizeof(cmdBuf));
            if (ImGui::InputText(("コマンド##" + std::to_string(i)).c_str(), cmdBuf, sizeof(cmdBuf)))
                module["command"] = std::string(cmdBuf);

            // 削除ボタン
            ImGui::SameLine();
            if (ImGui::Button(("削除##" + std::to_string(i)).c_str()))
            {
                deleteIndex = (int)i; // 後で削除
            }

            ImGui::Separator();
        }

        // 実際に削除（ループ中にeraseすると問題になるため後で）
        if (deleteIndex != -1)
        {
            moduleList["modules"].erase(moduleList["modules"].begin() + deleteIndex);
        }
    }
    else
    {
        ImGui::Text("moduleList.jsonに何も書かれていません");
    }

    // 新規追加ボタン
    if (ImGui::Button("新しいモジュールを追加"))
    {
        json newModule = {
            {"enabled", true},
            {"name", "新しいモジュール"},
            {"command", ""}
        };
        moduleList["modules"].push_back(newModule);
    }

    // 保存ボタン
    if (ImGui::Button("セーブ"))
    {
        if (SaveModuleList())
            std::cout << "moduleList.json を保存しました\n";
    }

    ImGui::End();

    if (!showModuleManager)
    {
        moduleList.clear();
        loaded = false;
    }
}

void LaunchModules()
{
    // --- 実行ファイルのディレクトリを取得 ---
    char pathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, pathBuf, MAX_PATH);
    std::string fullPath(pathBuf);
    size_t lastSlash = fullPath.find_last_of("\\/");
    std::string appDir = fullPath.substr(0, lastSlash + 1);

    // --- moduleList.json のパス ---
    std::string moduleListPath = appDir + "assets\\moduleList.json";

    // --- JSONを読み込み ---
    std::ifstream file(moduleListPath);
    if (!file.is_open())
    {
        std::cerr << "moduleList.json が開けませんでした: " << moduleListPath << std::endl;
        return;
    }

    json moduleList;
    file >> moduleList;
    file.close();

    // --- モジュールを順に起動 ---
    for (auto& module : moduleList["modules"])
    {
        bool enabled = module.value("enabled", true);
        if (!enabled) continue;

        std::string name = module.value("name", "unknown");
        std::string command = module.value("command", "");

        if (command.empty())
        {
            std::cerr << "モジュール '" << name << "' のコマンドが設定されていません。" << std::endl;
            continue;
        }

        std::cout << "モジュール '" << name << "' を起動中: " << command << std::endl;

        // --- 非同期で起動（コンソールにブロックされない） ---
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcessA(
            NULL, (LPSTR)command.c_str(),
            NULL, NULL, FALSE,
            CREATE_NO_WINDOW, NULL, NULL,
            &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            std::cerr << "モジュール '" << name << "' の起動に失敗しました。" << std::endl;
        }
    }
}


std::string MatToBase64(const cv::Mat& mat)
{
    // 元は BGRA
    CV_Assert(mat.type() == CV_8UC4);
    CV_Assert(mat.isContinuous());

    cv::Mat rgbaMat;
    cv::cvtColor(mat, rgbaMat, cv::COLOR_BGRA2RGBA); // B G R A → R G B A

    const unsigned char* data = rgbaMat.data;
    size_t dataSize = rgbaMat.total() * rgbaMat.elemSize();

    return Base64::Encode(data, dataSize);
}

bool can_bind(const std::string& ip, unsigned short port)
{
    try {
        asio::io_context ctx;
        asio::ip::tcp::acceptor acceptor(ctx);
        asio::ip::tcp::endpoint ep(asio::ip::make_address(ip), port);
        acceptor.open(ep.protocol());
        acceptor.bind(ep);  // ここでポート使用中なら例外
        acceptor.close();
        return true; // バインド可能
    }
    catch (const std::exception&) {
        return false; // 既に使用中などでバインド不可
    }
}


void runServerThread(const std::string& ipAddress, unsigned short port)
{
    wsServer.init_asio();
    wsServer.set_reuse_addr(true);

    try {
        // ここで listen() する
        asio::ip::tcp::endpoint ep(asio::ip::make_address(ipAddress), port);
        wsServer.listen(ep);
        wsServer.start_accept();
    }
    catch (const websocketpp::exception& e) {
        std::cerr << "サーバー起動失敗: " << e.what() << std::endl;
        return;
    }

    std::cout << "サーバー起動成功: " << ipAddress << ":" << port << std::endl;

    while (running.load())
    {
        wsServer.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    wsServer.stop();
}






void setWindowIconFromExe(GLFWwindow *window)
{
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd)
        return;

    char exePath[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exePath, MAX_PATH))
        return;

    HICON hIconLarge = nullptr, hIconSmall = nullptr;
    ExtractIconExA(exePath, 0, &hIconLarge, &hIconSmall, 1);

    if (!hIconLarge && !hIconSmall)
        return;

    // --- 大アイコン設定（タスクバー用） ---
    if (hIconLarge)
    {
        int cx = GetSystemMetrics(SM_CXICON);
        int cy = GetSystemMetrics(SM_CYICON);
        HICON hLargeCopy = (HICON)CopyImage(hIconLarge, IMAGE_ICON, cx, cy, 0);
        if (hLargeCopy)
        {
            HICON prev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hLargeCopy);
            if (prev && prev != hIconSmall)
                DestroyIcon(prev);
            // hLargeCopy は OS が参照するのでアプリ終了時に破棄
        }
        DestroyIcon(hIconLarge); // ExtractIconExA で得た元ハンドルは破棄
    }

    // --- 小アイコン設定（タイトルバー用） ---
    if (hIconSmall)
    {
        int cx = GetSystemMetrics(SM_CXSMICON);
        int cy = GetSystemMetrics(SM_CYSMICON);
        HICON hSmallCopy = (HICON)CopyImage(hIconSmall, IMAGE_ICON, cx, cy, 0);
        if (hSmallCopy)
        {
            HICON prev = (HICON)SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hSmallCopy);
            if (prev)
                DestroyIcon(prev);
            // hSmallCopy も OS が参照するのでアプリ終了時に破棄
        }
        DestroyIcon(hIconSmall);
    }
}

using namespace cv;

std::atomic<bool> abort_fetch{false};

std::atomic<bool> stopThread{false};

// INIファイルの絶対パスを格納するグローバル変数
std::string iniPath;

// アプリケーション設定をINIファイルに保存する関数
void SaveAppSettings()
{
    std::ofstream ofs(iniPath);
    if (!ofs.is_open())
    {
        std::cerr << "INIファイルを保存できませんでした: " << iniPath << std::endl;
        return;
    }

    // ウィンドウの表示状態
    ofs << "showOriginal=" << showOriginal << std::endl;
    ofs << "showRealtime=" << showRealtime << std::endl;
    ofs << "showDiff=" << showDiff << std::endl;
    ofs << "showSettings=" << showSettings << std::endl;
    ofs << "showInfo=" << showInfo << std::endl;
	ofs << "showWindowManager=" << showWindowManager << std::endl;
	ofs << "showModuleSeting=" << showModuleSeting << std::endl;

    // 位置系
    ofs << "tile_x=" << tile_x << std::endl;
    ofs << "tile_y=" << tile_y << std::endl;
    ofs << "pixel_x=" << pixel_x << std::endl;
    ofs << "pixel_y=" << pixel_y << std::endl;
    ofs << "UpdateSpeed=" << UpdateSpeed << std::endl;
    ofs << "isLinkZoom=" << isLinkZoom << std::endl;
    // 差分形式
    ofs << "diffPattern=" << static_cast<int>(diffPattern.type) << std::endl;
    ofs << "MaskColor="
        << MaskColor.x << ","
        << MaskColor.y << ","
        << MaskColor.z << ","
        << MaskColor.w << std::endl;
    // パス
    ofs << "path=" << szFile << std::endl;
    // サーバー
	ofs << "useServer=" << useServer << std::endl;
	ofs << "autoStartModules=" << autoStartModules << std::endl;
	ofs << "serverIP=" << serverIP << std::endl;
	ofs << "serverPort=" << serverPort << std::endl;
    // アップデート
	ofs << "foundUpdate=" << foundUpdate << std::endl;
	ofs << "latestVersion=" << latestVersion << std::endl;
	ofs << "updateURL=" << updateURL << std::endl;
	ofs << "showUpdateMessage=" << showUpdateMessage << std::endl;

    ofs.close();
}

// アプリケーション設定をINIファイルから読み込む関数
void LoadAppSettings()
{
    std::ifstream ifs(iniPath);
    if (!ifs.is_open())
    {
        std::cerr << "INIファイルが見つかりません: " << iniPath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        size_t pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        try
        {
            if (key == "showOriginal")
                showOriginal = (std::stoi(val) != 0);
            else if (key == "showRealtime")
                showRealtime = (std::stoi(val) != 0);
            else if (key == "showDiff")
                showDiff = (std::stoi(val) != 0);
            else if (key == "showSettings")
                showSettings = (std::stoi(val) != 0);
            else if (key == "showInfo")
                showInfo = (std::stoi(val) != 0);
			else if (key == "showWindowManager")
				showWindowManager = (std::stoi(val) != 0);
			else if (key == "showModuleSeting")
				showModuleSeting = (std::stoi(val) != 0);
            else if (key == "tile_x")
                tile_x = std::stoi(val);
            else if (key == "tile_y")
                tile_y = std::stoi(val);
            else if (key == "pixel_x")
                pixel_x = std::stoi(val);
            else if (key == "pixel_y")
                pixel_y = std::stoi(val);
            else if (key == "UpdateSpeed")
                UpdateSpeed = std::stof(val);
            else if (key == "isLinkZoom")
                isLinkZoom = (std::stoi(val) != 0);
            else if (key == "diffPattern")
                diffPattern = DiffPattern(static_cast<DiffPatternType>(std::stoi(val)));
            else if (key == "MaskColor")
            {
                sscanf_s(val.c_str(), "%f,%f,%f,%f", &MaskColor.x, &MaskColor.y, &MaskColor.z, &MaskColor.w);
            }
            else if (key == "path")
                szFile = val;
			else if (key == "useServer")
				useServer = (std::stoi(val) != 0);
			else if (key == "autoStartModules")
				autoStartModules = (std::stoi(val) != 0);
			else if (key == "serverIP")
				serverIP = val;
			else if (key == "serverPort")
				serverPort = static_cast<unsigned short>(std::stoi(val));
			else if (key == "foundUpdate")
				foundUpdate = (std::stoi(val) != 0);
			else if (key == "latestVersion")
                latestVersion = val;
            else if (key == "updateURL")
				updateURL = val;
			else if (key == "showUpdateMessage")
				showUpdateMessage = (std::stoi(val) != 0);
        }
        catch (const std::exception &e)
        {
            std::cerr << "INI file parse error: " << e.what() << std::endl;
        }
    }
    ifs.close();
}

cv::Mat applyAlphaMask(const cv::Mat &src, const cv::Mat &mask)
{
    if (src.empty() || mask.empty())
        return cv::Mat();
    CV_Assert(src.size() == mask.size());
    CV_Assert(src.type() == CV_8UC4 && mask.type() == CV_8UC4);

    cv::Mat dst = src.clone();
    for (int y = 0; y < src.rows; ++y)
    {
        const cv::Vec4b *srcRow = src.ptr<cv::Vec4b>(y);
        const cv::Vec4b *maskRow = mask.ptr<cv::Vec4b>(y);
        cv::Vec4b *dstRow = dst.ptr<cv::Vec4b>(y);
        for (int x = 0; x < src.cols; ++x)
        {
            uchar srcA = srcRow[x][3] ? 255 : 0;
            uchar maskA = maskRow[x][3] ? 255 : 0;
            dstRow[x][3] = srcA & maskA;
        }
    }
    return dst;
}

void ensureBGRA(cv::Mat &img)
{
    if (img.empty())
        return;
    if (img.channels() == 3)
        cv::cvtColor(img, img, COLOR_BGR2BGRA);
}

// 拡張版
std::tuple<cv::Mat, int, int> imageDifferenceSafe(
    const cv::Mat &img1,
    const cv::Mat &img2,
    const DiffPattern &pattern)
{
    if (img1.empty() || img2.empty())
        return {cv::Mat(), 0, 0};

    int w = std::min(img1.cols, img2.cols);
    int h = std::min(img1.rows, img2.rows);
    if (w <= 0 || h <= 0)
        return {cv::Mat(), 0, 0};

    cv::Mat diffImage = cv::Mat::zeros(img1.size(), CV_8UC4);
    cv::Rect roi(0, 0, w, h);
    cv::Mat roi1 = img1(roi);
    cv::Mat roi2 = img2(roi);

    // アルファチャンネル
    cv::Mat alpha;
    if (roi1.channels() == 4)
        cv::extractChannel(roi1, alpha, 3);
    else
        alpha = cv::Mat::ones(roi.size(), CV_8U) * 255;

    // 差分計算
    cv::Mat merged = cv::Mat::zeros(roi.size(), CV_8UC4);
    int changedPixels = 0;

    for (int y = 0; y < h; ++y)
    {
        const cv::Vec4b *p1 = roi1.ptr<cv::Vec4b>(y);
        const cv::Vec4b *p2 = roi2.ptr<cv::Vec4b>(y);
        const uchar *maskPtr = alpha.ptr<uchar>(y);
        cv::Vec4b *dst = merged.ptr<cv::Vec4b>(y);

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
                                       (uchar)(MaskColor.z * 255),
                                       (uchar)(MaskColor.y * 255),
                                       (uchar)(MaskColor.x * 255),
                                       (uchar)(MaskColor.w * 255))
                                 : cv::Vec4b(0, 0, 0, 255);
                break;

            case DiffPatternType::OverlayTemplate:
                if (changed)
                    dst[x] = cv::Vec4b(
                        (uchar)(MaskColor.z * 255),
                        (uchar)(MaskColor.y * 255),
                        (uchar)(MaskColor.x * 255),
                        (uchar)(MaskColor.w * 255));
                else if (!img1.empty())
                    dst[x] = img1.at<cv::Vec4b>(y, x);
                else
                    dst[x] = p1[x];
                break;
            }
        }
    }

    merged.copyTo(diffImage(roi));
    int totalOpaquePixels = cv::countNonZero(alpha);
    return {diffImage, totalOpaquePixels, changedPixels};
}

cpr::Response cancellable_fetch(const std::string &url, const cpr::Header &headers)
{
    try
    {
        auto future = cpr::GetAsync(cpr::Url{url}, headers, cpr::Timeout{5000});

        while (!abort_fetch && future.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready)
        {
        }

        if (abort_fetch)
        {
            try
            {
                // 戻り値を変数に格納して破棄
                cpr::Response ignored = future.get();
                (void)ignored; // 明示的に破棄
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

cv::Mat fetch_tiles_and_crop_cpp(
    int tile_x, int tile_y, int x_in_tile, int y_in_tile,
    int ref_width, int ref_height, int TILE_SIZE = 1000)
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
        {"Referer", "https://www.google.com/"}};

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
            cpr::Response r = cancellable_fetch(url, headers);
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

GLuint matToTexture(const cv::Mat &mat)
{
    if (mat.empty())
        return 0;
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    GLenum format = (mat.channels() == 3) ? GL_BGR : GL_BGRA;
    GLint internalFormat = (mat.channels() == 3) ? GL_RGB8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, mat.cols, mat.rows, 0, format, GL_UNSIGNED_BYTE, mat.data);
    float borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

enum class ZoomDir
{
    ZoomIn,
    ZoomOut
};

void ApplyZoom(ImVec2 &uv0, ImVec2 &uv1, const ImVec2 &origin, ZoomDir dir, float factor = 1.1f)
{
    float zoom = (dir == ZoomDir::ZoomIn) ? factor : (1.0f / factor);
    uv0.x = (uv0.x - origin.x) * zoom + origin.x;
    uv0.y = (uv0.y - origin.y) * zoom + origin.y;
    uv1.x = (uv1.x - origin.x) * zoom + origin.x;
    uv1.y = (uv1.y - origin.y) * zoom + origin.y;
}

void CalcUVForAspect(float imgAspect, float winAspect, ImVec2 &uv0, ImVec2 &uv1)
{
    ImVec2 uvCenter((uv0.x + uv1.x) * 0.5f, (uv0.y + uv1.y) * 0.5f);
    float uvWidth = uv1.x - uv0.x;
    float uvHeight = uv1.y - uv0.y;

    if (imgAspect < winAspect)
    {
        float newUvWidth = (winAspect / imgAspect) * uvHeight;
        uv0.x = uvCenter.x - newUvWidth * 0.5f;
        uv1.x = uvCenter.x + newUvWidth * 0.5f;
    }
    else if (imgAspect > winAspect)
    {
        float newUvHeight = (imgAspect / winAspect) * uvWidth;
        uv0.y = uvCenter.y - newUvHeight * 0.5f;
        uv1.y = uvCenter.y + newUvHeight * 0.5f;
    }
}

int main()
{
    SetupConsoleIfDebug();
    // 実行ファイルのディレクトリを取得し、INIファイルの絶対パスを作成
    char pathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, pathBuf, MAX_PATH);
    std::string fullPath(pathBuf);
    size_t lastSlash = fullPath.find_last_of("\\/");
    std::string appDir = fullPath.substr(0, lastSlash + 1);
    iniPath = appDir + "app_settings.ini";

    std::string imguiIniPath = appDir + "imgui.ini";


    if (!glfwInit())
        return -1;
    GLFWwindow *window = glfwCreateWindow(1200, 800, "WP-Guardian", nullptr, nullptr);
    glfwMaximizeWindow(window);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    setWindowIconFromExe(window);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = imguiIniPath.c_str();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    ImFontConfig font_cfg{};
    font_cfg.FontDataOwnedByAtlas = true;

    ImFont *font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/meiryo.ttc", 18.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
    ImFont *bigFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/meiryo.ttc", 25.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
    if (!font)
        std::cerr << "フォント読み込み失敗！\n";
    if (!bigFont)
        std::cerr << "フォント読み込み失敗！\n";

    // アプリ起動時に設定を読み込む
    LoadAppSettings();
    std::thread serverThread;

    // useServerがtrueの場合にのみ、ポートの空き状況をチェック
    if (useServer) {
        // 1. ポートが使用可能かメインスレッドで先に確認する
        bool isPortAvailable = can_bind(serverIP, serverPort);

        if (isPortAvailable) {
            // ポートが空いているので、サーバーを別スレッドで起動
            // runServerThreadはwsServer.poll()でループするため、スレッドで実行
            serverThread = std::thread(runServerThread, serverIP, serverPort);
            // サーバー起動スレッドのハンドルを適切に管理（detachまたはjoin）
        }
        else {
            // 2. ポートが使用中のため、サーバーは起動せず、フラグを更新
            std::cerr << "ポートがすでに使用されています: " << serverPort << "。サーバーを起動しません。" << std::endl;
            // サーバーが起動しないため、関連するモジュールの自動起動も無効化する
            useServer = false;
            autoStartModules = false;
            // ※このフラグ変更は、runServerThread内で実行されていた処理と同じです。
        }
    }
    ImWS imws(wsServer);
    imws.set_server_handlers();
    // 3. 最終的に確定した autoStartModules の値に基づいてモジュールを起動
    if (autoStartModules) {
        LaunchModules();
    }

    if (showUpdateMessage) {
        checkUpdate();
    }
    showVersion = foundUpdate && showUpdateMessage;
    // szFileBufferにszFileの値をコピーして、ImGuiの初期値を設定
    strncpy(szFileBuffer, szFile.c_str(), MAX_PATH - 1);
    szFileBuffer[MAX_PATH - 1] = '\0';
	szFileStr = szFile;

    cv::Mat originalImg = cv::imread(szFile, cv::IMREAD_UNCHANGED);
    if (originalImg.empty())
    {
        originalImg = cv::Mat(1, 1, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    }
    ensureBGRA(originalImg);
    GLuint originalTexID = matToTexture(originalImg);
	std::string originalBase64 = MatToBase64(originalImg);
    int width = originalImg.cols;
    int height = originalImg.rows;

    cv::Mat realtimeImg(height, width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    GLuint realtimeTexID = matToTexture(realtimeImg);
	std::string realtimeBase64 = MatToBase64(realtimeImg);

    cv::Mat emptyDiff(height, width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    GLuint diffTexID = matToTexture(emptyDiff);
	std::string diffBase64 = MatToBase64(emptyDiff);

	imws.register_readonly_variable("width", width);
	imws.register_readonly_variable("height", height);
	imws.register_readonly_variable("originalImage", originalBase64);
	imws.register_readonly_variable("realtimeImage", realtimeBase64);
	imws.register_readonly_variable("diffImage", diffBase64);

    static int tmpTile_x = tile_x;
    static int tmpTile_y = tile_y;
    static int tmpPixel_x = pixel_x;
    static int tmpPixel_y = pixel_y;
    static float tmpUpdateSpeed = UpdateSpeed;
    static bool tmpIsLinkZoom = isLinkZoom;
    static DiffPattern tmpDiffPattern = diffPattern;
    static ImVec4 tmpMaskColor = MaskColor;
    static bool tmpAutoStartModules = autoStartModules;
	static int tmpServerPort = serverPort;
    static std::string tmpServerIP = serverIP;
	static char bufTmpServerIP[128];
	strncpy(bufTmpServerIP, tmpServerIP.c_str(),sizeof(bufTmpServerIP) - 1);
    bufTmpServerIP[sizeof(bufTmpServerIP) - 1] = '\0';
	static bool tmpUseServer = useServer;


	imws.register_writable_variable("tmpTile_x", tmpTile_x);
	imws.register_writable_variable("tmpTile_y", tmpTile_y);
	imws.register_writable_variable("tmpPixel_x", tmpPixel_x);
	imws.register_writable_variable("tmpPixel_y", tmpPixel_y);
	imws.register_writable_variable("tmpUpdateSpeed", tmpUpdateSpeed);
	imws.register_writable_variable("tmpDiffPattern", tmpDiffPattern);
	imws.register_writable_variable("tmpMaskColor", tmpMaskColor);
    imws.register_writable_variable("tmpPath", szFileStr);


    imws.register_readonly_variable("tmpTile_x", tmpTile_x);
	imws.register_readonly_variable("tmpTile_y", tmpTile_y);
	imws.register_readonly_variable("tmpPixel_x", tmpPixel_x);
	imws.register_readonly_variable("tmpPixel_y", tmpPixel_y);
	imws.register_readonly_variable("tmpUpdateSpeed", tmpUpdateSpeed);
	imws.register_readonly_variable("tmpDiffPattern", tmpDiffPattern);
	imws.register_readonly_variable("tmpMaskColor", tmpMaskColor);
	imws.register_readonly_variable("tmpPath", szFileStr);


	imws.register_readonly_variable("tile_x", tile_x);
	imws.register_readonly_variable("tile_y", tile_y);
	imws.register_readonly_variable("pixel_x", pixel_x);
	imws.register_readonly_variable("pixel_y", pixel_y);
	imws.register_readonly_variable("UpdateSpeed", UpdateSpeed);
	imws.register_readonly_variable("diffPattern", diffPattern);
	imws.register_readonly_variable("MaskColor", MaskColor);
	imws.register_readonly_variable("path", szFile);

    double diffPercent = 0.0;
    int totalOpaquePixels = 0;
    int changedPixels = 0;

	imws.register_readonly_variable("totalPixels", totalOpaquePixels);
	imws.register_readonly_variable("wrongPixels", changedPixels);

    imws.register_readonly_variable("diffPercent", diffPercent);

    std::mutex imgMutex;
    std::condition_variable cv_newFrame;
    bool newFrameReady = false;

    ImVec2 OriginalUV0(0, 0), OriginalUV1(1, 1);
    ImVec2 OriginalOrigin;
    static bool isPanning = false;
    static ImVec2 lastMouse;
    ImVec2 RealtimeUV0(0, 0), RealtimeUV1(1, 1);
    ImVec2 RealtimeOrigin;
    bool isPanningRealtime = false;
    ImVec2 lastMouseRealtime;

    ImVec2 DiffUV0(0, 0), DiffUV1(1, 1);
    ImVec2 DiffOrigin;
    bool isPanningDiff = false;
    ImVec2 lastMouseDiff;

	bool update = false;
	imws.register_writable_variable("update", update);

    std::thread updateThread([&]()
                             {
        while(!stopThread){
            cv::Mat newImg = fetch_tiles_and_crop_cpp(tile_x,tile_y,pixel_x,pixel_y,width,height);
            if (!newImg.empty()) {
                newImg = applyAlphaMask(newImg, originalImg);
                std::lock_guard<std::mutex> lock(imgMutex);
                realtimeImg = newImg.clone();
                auto [diffImg, totalOpaque, changed] = imageDifferenceSafe(originalImg, realtimeImg, diffPattern);
                diffPercent = (totalOpaque > 0) ? (double)changed / totalOpaque * 100.0 : 0.0;
                totalOpaquePixels = totalOpaque;
                changedPixels = changed;

                emptyDiff = diffImg.clone();
                newFrameReady = true;
                cv_newFrame.notify_one();
            }
            for(int i=0;i<10 && !stopThread;i++) std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(UpdateSpeed * 100)));
        } });

    glfwSetWindowCloseCallback(window, [](GLFWwindow *win)
                               {
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        abort_fetch = true;
        stopThread = true; });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(viewport->ID, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("ウィンドウ"))
            {
                ImGui::MenuItem("オリジナル画像", nullptr, &showOriginal);
                ImGui::MenuItem("リアルタイム画像", nullptr, &showRealtime);
                ImGui::MenuItem("差分画像", nullptr, &showDiff);
                ImGui::MenuItem("設定", nullptr, &showSettings);
                ImGui::MenuItem("情報", nullptr, &showInfo);
				ImGui::MenuItem("ウィンドウ管理", nullptr, &showWindowManager);
				ImGui::MenuItem("モジュール設定", nullptr, &showModuleSeting);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("詳細"))
            {
                ImGui::MenuItem("アップデートを確認", nullptr, &showVersion);
				ImGui::MenuItem("モジュール管理", nullptr, &showModuleManager);
                ImGui::EndMenu();
			}
            ImGui::EndMainMenuBar();
        }

        imws.update();
		imws.drawWindowManager(showWindowManager);
        ShowModuleManagerWindow();
        showVersionWindow();

        if (showOriginal)
        {
            ImGui::Begin("オリジナル画像", &showOriginal);

            ImVec2 winSize = ImGui::GetContentRegionAvail();
            ImVec2 uv0 = OriginalUV0;
            ImVec2 uv1 = OriginalUV1;

            float imgAspect = (float)width / height;
            float winAspect = winSize.x / winSize.y;
            CalcUVForAspect(imgAspect, winAspect, uv0, uv1);

            if (ImGui::IsWindowHovered())
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 relativeMouse;
                relativeMouse.x = mousePos.x - windowPos.x;
                relativeMouse.y = mousePos.y - windowPos.y;
                OriginalOrigin.x = uv0.x + (uv1.x - uv0.x) * (relativeMouse.x / winSize.x);
                OriginalOrigin.y = uv0.y + (uv1.y - uv0.y) * (relativeMouse.y / winSize.y);

                if (io.MouseWheel > 0)
                {
                    ApplyZoom(uv0, uv1, OriginalOrigin, ZoomDir::ZoomOut);
                    if (isLinkZoom)
                    {
                        ApplyZoom(RealtimeUV0, RealtimeUV1, OriginalOrigin, ZoomDir::ZoomOut);
                        ApplyZoom(DiffUV0, DiffUV1, OriginalOrigin, ZoomDir::ZoomOut);
                    }
                }
                else if (io.MouseWheel < 0)
                {
                    ApplyZoom(uv0, uv1, OriginalOrigin, ZoomDir::ZoomIn);
                    if (isLinkZoom)
                    {
                        ApplyZoom(RealtimeUV0, RealtimeUV1, OriginalOrigin, ZoomDir::ZoomIn);
                        ApplyZoom(DiffUV0, DiffUV1, OriginalOrigin, ZoomDir::ZoomIn);
                    }
                }
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    if (!isPanning)
                    {
                        isPanning = true;
                        lastMouse = mousePos;
                    }
                    ImVec2 delta = ImVec2(
                        (mousePos.x - lastMouse.x) / winSize.x * (uv1.x - uv0.x),
                        (mousePos.y - lastMouse.y) / winSize.y * (uv1.y - uv0.y));
                    uv0.x -= delta.x;
                    uv1.x -= delta.x;
                    uv0.y -= delta.y;
                    uv1.y -= delta.y;
                    if (isLinkZoom)
                    {
                        RealtimeUV0.x -= delta.x;
                        RealtimeUV1.x -= delta.x;
                        RealtimeUV0.y -= delta.y;
                        RealtimeUV1.y -= delta.y;
                        DiffUV0.x -= delta.x;
                        DiffUV1.x -= delta.x;
                        DiffUV0.y -= delta.y;
                        DiffUV1.y -= delta.y;
                    }
                    lastMouse = mousePos;
                }
                else
                {
                    isPanning = false;
                }
            }
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                // UVを初期状態に戻す
                uv0 = ImVec2(0, 0);
                uv1 = ImVec2(1, 1);
                if (isLinkZoom)
                {
                    RealtimeUV0 = ImVec2(0, 0);
                    RealtimeUV1 = ImVec2(1, 1);
                    DiffUV0 = ImVec2(0, 0);
                    DiffUV1 = ImVec2(1, 1);
                }
                CalcUVForAspect(imgAspect, winAspect, uv0, uv1);
            }

            // 更新されたUVを保存
            OriginalUV0 = uv0;
            OriginalUV1 = uv1;

            ImGui::Image((void *)(intptr_t)originalTexID, winSize, OriginalUV0, OriginalUV1);
            ImGui::End();
        }

        if (showRealtime)
        {
            ImGui::Begin("リアルタイム画像", &showRealtime);

            ImVec2 winSize = ImGui::GetContentRegionAvail();
            ImVec2 uv0 = RealtimeUV0;
            ImVec2 uv1 = RealtimeUV1;

            float imgAspect = (float)width / height;
            float winAspect = winSize.x / winSize.y;
            CalcUVForAspect(imgAspect, winAspect, uv0, uv1);

            if (ImGui::IsWindowHovered())
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 relativeMouse(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
                RealtimeOrigin.x = uv0.x + (uv1.x - uv0.x) * (relativeMouse.x / winSize.x);
                RealtimeOrigin.y = uv0.y + (uv1.y - uv0.y) * (relativeMouse.y / winSize.y);

                if (io.MouseWheel > 0)
                {
                    ApplyZoom(uv0, uv1, RealtimeOrigin, ZoomDir::ZoomOut);
                    if (isLinkZoom)
                    {
                        ApplyZoom(OriginalUV0, OriginalUV1, RealtimeOrigin, ZoomDir::ZoomOut);
                        ApplyZoom(DiffUV0, DiffUV1, RealtimeOrigin, ZoomDir::ZoomOut);
                    }
                }
                else if (io.MouseWheel < 0)
                {
                    ApplyZoom(uv0, uv1, RealtimeOrigin, ZoomDir::ZoomIn);
                    if (isLinkZoom)
                    {
                        ApplyZoom(OriginalUV0, OriginalUV1, RealtimeOrigin, ZoomDir::ZoomIn);
                        ApplyZoom(DiffUV0, DiffUV1, RealtimeOrigin, ZoomDir::ZoomIn);
                    }
                }
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    if (!isPanningRealtime)
                    {
                        isPanningRealtime = true;
                        lastMouseRealtime = mousePos;
                    }
                    ImVec2 delta(
                        (mousePos.x - lastMouseRealtime.x) / winSize.x * (uv1.x - uv0.x),
                        (mousePos.y - lastMouseRealtime.y) / winSize.y * (uv1.y - uv0.y));
                    uv0.x -= delta.x;
                    uv1.x -= delta.x;
                    uv0.y -= delta.y;
                    uv1.y -= delta.y;
                    if (isLinkZoom)
                    {
                        OriginalUV0.x -= delta.x;
                        OriginalUV1.x -= delta.x;
                        OriginalUV0.y -= delta.y;
                        OriginalUV1.y -= delta.y;
                        DiffUV0.x -= delta.x;
                        DiffUV1.x -= delta.x;
                        DiffUV0.y -= delta.y;
                        DiffUV1.y -= delta.y;
                    }
                    lastMouseRealtime = mousePos;
                }
                else
                {
                    isPanningRealtime = false;
                }
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                uv0 = ImVec2(0, 0);
                uv1 = ImVec2(1, 1);
                if (isLinkZoom)
                {
                    OriginalUV0 = ImVec2(0, 0);
                    OriginalUV1 = ImVec2(1, 1);
                    DiffUV0 = ImVec2(0, 0);
                    DiffUV1 = ImVec2(1, 1);
                }
                CalcUVForAspect(imgAspect, winAspect, uv0, uv1);
            }

            RealtimeUV0 = uv0;
            RealtimeUV1 = uv1;

            ImGui::Image((void *)(intptr_t)realtimeTexID, winSize, RealtimeUV0, RealtimeUV1);
            ImGui::End();
        }

        if (showDiff)
        {
            ImGui::Begin("差分画像", &showDiff);

            ImVec2 winSize = ImGui::GetContentRegionAvail();
            ImVec2 uv0 = DiffUV0;
            ImVec2 uv1 = DiffUV1;

            float imgAspect = (float)width / height;
            float winAspect = winSize.x / winSize.y;
            CalcUVForAspect(imgAspect, winAspect, uv0, uv1);

            if (ImGui::IsWindowHovered())
            {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 relativeMouse(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
                DiffOrigin.x = uv0.x + (uv1.x - uv0.x) * (relativeMouse.x / winSize.x);
                DiffOrigin.y = uv0.y + (uv1.y - uv0.y) * (relativeMouse.y / winSize.y);

                if (io.MouseWheel > 0)
                {
                    ApplyZoom(uv0, uv1, DiffOrigin, ZoomDir::ZoomOut);
                    if (isLinkZoom)
                    {
                        ApplyZoom(RealtimeUV0, RealtimeUV1, DiffOrigin, ZoomDir::ZoomOut);
                        ApplyZoom(OriginalUV0, OriginalUV1, DiffOrigin, ZoomDir::ZoomOut);
                    }
                }
                else if (io.MouseWheel < 0)
                {
                    ApplyZoom(uv0, uv1, DiffOrigin, ZoomDir::ZoomIn);
                    if (isLinkZoom)
                    {
                        ApplyZoom(RealtimeUV0, RealtimeUV1, DiffOrigin, ZoomDir::ZoomIn);
                        ApplyZoom(OriginalUV0, OriginalUV1, DiffOrigin, ZoomDir::ZoomIn);
                    }
                }

                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    if (!isPanningDiff)
                    {
                        isPanningDiff = true;
                        lastMouseDiff = mousePos;
                    }
                    ImVec2 delta(
                        (mousePos.x - lastMouseDiff.x) / winSize.x * (uv1.x - uv0.x),
                        (mousePos.y - lastMouseDiff.y) / winSize.y * (uv1.y - uv0.y));
                    uv0.x -= delta.x;
                    uv1.x -= delta.x;
                    uv0.y -= delta.y;
                    uv1.y -= delta.y;
                    if (isLinkZoom)
                    {
                        RealtimeUV0.x -= delta.x;
                        RealtimeUV1.x -= delta.x;
                        RealtimeUV0.y -= delta.y;
                        RealtimeUV1.y -= delta.y;
                        OriginalUV0.x -= delta.x;
                        OriginalUV1.x -= delta.x;
                        OriginalUV0.y -= delta.y;
                        OriginalUV1.y -= delta.y;
                    }
                    lastMouseDiff = mousePos;
                }
                else
                {
                    isPanningDiff = false;
                }
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                uv0 = ImVec2(0, 0);
                uv1 = ImVec2(1, 1);
                if (isLinkZoom)
                {
                    RealtimeUV0 = ImVec2(0, 0);
                    RealtimeUV1 = ImVec2(1, 1);
                    OriginalUV0 = ImVec2(0, 0);
                    OriginalUV1 = ImVec2(1, 1);
                }
                CalcUVForAspect(imgAspect, winAspect, uv0, uv1);
            }

            DiffUV0 = uv0;
            DiffUV1 = uv1;

            ImGui::Image((void *)(intptr_t)diffTexID, winSize, DiffUV0, DiffUV1);
            ImGui::End();
        }

        if (showSettings)
        {
            ImGui::Begin("設定", &showSettings);
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float itemWidth = ImGui::GetContentRegionAvail().x * 0.9f;
            float xPos = (windowWidth - itemWidth) * 0.5f;

            ImGui::SetCursorPosX(xPos);
            ImGui::PushFont(bigFont);
            ImGui::Text("座標");
            ImGui::PopFont();
            ImGui::PushItemWidth(itemWidth);
            ImGui::SetCursorPosX(xPos);
            ImGui::InputInt("Tl X", &tmpTile_x);
            ImGui::SetCursorPosX(xPos);
            ImGui::InputInt("Tl Y", &tmpTile_y);
            ImGui::SetCursorPosX(xPos);
            ImGui::InputInt("Px X", &tmpPixel_x);
            ImGui::SetCursorPosX(xPos);
            ImGui::InputInt("Px Y", &tmpPixel_y);
            ImGui::PopItemWidth();
            ImGui::Spacing();

            ImGui::SetCursorPosX(xPos);
            ImGui::PushFont(bigFont);
            ImGui::Text("更新間隔");
            ImGui::PopFont();
            ImGui::PushItemWidth(itemWidth);
            ImGui::SetCursorPosX(xPos);
            ImGui::InputFloat("秒", &tmpUpdateSpeed, 0.1f, 1.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Spacing();

            ImGui::PushItemWidth(itemWidth);
            ImGui::SetCursorPosX(xPos);
            ImGui::Checkbox("リンク拡大", &tmpIsLinkZoom);
            ImGui::PopItemWidth();
            ImGui::Spacing();

            ImGui::SetCursorPosX(xPos);
            ImGui::PushFont(bigFont);
            ImGui::Text("差分形式");
            ImGui::PopFont();
            ImGui::PushItemWidth(itemWidth);
            ImGui::SetCursorPosX(xPos);
            ImGui::Combo("##差分形式", (int *)&tmpDiffPattern.type, "差分計算表示\0差分部分を強調色\0差分以外をテンプレ表示\0");

            ImGui::PopItemWidth();
            ImGui::Spacing();

            if (tmpDiffPattern.type == DiffPatternType::ColorMask || tmpDiffPattern.type == DiffPatternType::OverlayTemplate)
            {
                ImGui::SetCursorPosX(xPos);
                ImGui::PushFont(bigFont);
                ImGui::Text("使用する色");
                ImGui::PopFont();
                ImGui::PushItemWidth(itemWidth);
                ImGui::SetCursorPosX(xPos);
                ImGui::ColorEdit4("##マスク色", (float *)&tmpMaskColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                ImGui::PopItemWidth();
                ImGui::Spacing();
            }

            ImGui::SetCursorPosX(xPos);
            ImGui::PushFont(bigFont);
            ImGui::Text("使用画像パス");
            ImGui::PopFont();
            ImGui::PushItemWidth(itemWidth - 80); // ボタン幅80px確保
            ImGui::SetCursorPosX(xPos);
            // szFileBufferを使用
            if (ImGui::InputText("##画像保存先", szFileBuffer, MAX_PATH))
            {
				szFileStr = szFileBuffer;
            }
            else
            {
                strncpy(szFileBuffer, szFileStr.c_str(), MAX_PATH - 1);
                szFileBuffer[MAX_PATH - 1] = '\0';

            }
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::SetCursorPosX(xPos + itemWidth - 80); // ボタンの開始位置
            if (ImGui::Button("参照", ImVec2(80, 0)))
            {
                OPENFILENAME ofn;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);

                HWND hwnd = glfwGetWin32Window(window);
                ofn.hwndOwner = hwnd;

                // ImGuiバッファを使用
                ofn.lpstrFile = szFileBuffer;
                ofn.nMaxFile = sizeof(szFileBuffer);
                ofn.lpstrFilter = "PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileName(&ofn))
                {
                    // ダイアログでパスが選択されたら、std::string変数にコピー
                    szFileStr = szFileBuffer;
                }
            }
            ImGui::Spacing();

            ImGui::SetCursorPosX(xPos);
            if (ImGui::Button("更新", ImVec2(itemWidth, 0)) || update)
            {
                std::lock_guard<std::mutex> lock(imgMutex);

                // ImGuiバッファからszFileに最新のパスをコピー
                szFile = szFileStr;

                if (!szFile.empty())
                {
                    cv::Mat newImg = cv::imread(szFile, cv::IMREAD_UNCHANGED);
                    if (!newImg.empty())
                    {
                        originalImg = newImg.clone();
                        ensureBGRA(originalImg);
                        if (originalTexID)
                            glDeleteTextures(1, &originalTexID);
                        originalTexID = matToTexture(originalImg);
						originalBase64 = MatToBase64(originalImg);
                        width = originalImg.cols;
                        height = originalImg.rows;
                        realtimeImg = cv::Mat(height, width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
                        emptyDiff = cv::Mat(height, width, CV_8UC4, cv::Scalar(0, 0, 0, 0));

                        if (realtimeTexID)
                            glDeleteTextures(1, &realtimeTexID);
                        realtimeTexID = matToTexture(realtimeImg);
						realtimeBase64 = MatToBase64(realtimeImg);

                        if (diffTexID)
                            glDeleteTextures(1, &diffTexID);
                        diffTexID = matToTexture(emptyDiff);
						diffBase64 = MatToBase64(emptyDiff);
                    }
                }

                tile_x = tmpTile_x;
                tile_y = tmpTile_y;
                pixel_x = tmpPixel_x;
                pixel_y = tmpPixel_y;
                UpdateSpeed = tmpUpdateSpeed;
                isLinkZoom = tmpIsLinkZoom;

                OriginalUV0 = ImVec2(0, 0);
                OriginalUV1 = ImVec2(1, 1);
                RealtimeUV0 = ImVec2(0, 0);
                RealtimeUV1 = ImVec2(1, 1);
                DiffUV0 = ImVec2(0, 0);
                DiffUV1 = ImVec2(1, 1);
                diffPattern = tmpDiffPattern;
                MaskColor = tmpMaskColor;

                abort_fetch = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                abort_fetch = false;

				update = false;
            }
            ImGui::End();
        }

        if (showInfo)
        {
            ImGui::Begin("情報", &showInfo);
            ImGui::PushFont(bigFont);
            ImGui::Text("差分率: %.2f%%", diffPercent);
            ImGui::Text("%d / %d", changedPixels, totalOpaquePixels);
            ImGui::PopFont();
            ImGui::End();
        }
        if (showModuleSeting)
        {
			ImGui::Begin("モジュール設定", &showModuleSeting);
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float itemWidth = ImGui::GetContentRegionAvail().x * 0.9f;
            float xPos = (windowWidth - itemWidth) * 0.5f;
            ImGui::PushItemWidth(itemWidth);
            ImGui::SetCursorPosX(xPos);
            ImGui::Text("モジュールの動作(主にサーバーに関する)設定です。"); 
            ImGui::SetCursorPosX(xPos);
			ImGui::Text("設定の変更を保存する際はアプリを再起動します");
			ImGui::Spacing();
            ImGui::SetCursorPosX(xPos);
            ImGui::Text("モジュールを使用するか");
            ImGui::SetCursorPosX(xPos);
			ImGui::Checkbox("##useServer", &tmpUseServer);
            if (tmpUseServer)
            {
                ImGui::SetCursorPosX(xPos);
                ImGui::Text("モジュールを自動で起動するか");
                ImGui::SetCursorPosX(xPos);
                ImGui::Checkbox("##autoStartModules", &tmpAutoStartModules);
                ImGui::SetCursorPosX(xPos);
                if (ImGui::Button("モジュールの管理"))
                {
                    showModuleManager = true;
                }
                ImGui::SetCursorPosX(xPos);
                ImGui::Text("WebSocketサーバー ポート番号:");
                ImGui::SetCursorPosX(xPos);
                if (ImGui::InputInt("##port", &tmpServerPort))
                {
                    if (tmpServerPort < 0)
                        tmpServerPort = 0;
                    else if (tmpServerPort > 65535)
                        tmpServerPort = 65535;
                }
                ImGui::SetCursorPosX(xPos);
                ImGui::Text("サーバーIPアドレス:");
                ImGui::SetCursorPosX(xPos);
                if (ImGui::InputText("##ip", bufTmpServerIP, 128))
                {
                    tmpServerIP = std::string(bufTmpServerIP);
                }
            }
            else
            {
                tmpAutoStartModules = false;
                ImGui::SetCursorPosX(xPos);
                ImGui::Text("モジュールを無効にすると、サーバー自体が起動しなくなります。");
            }
            ImGui::PopItemWidth();
            ImGui::SetCursorPosX(xPos);
            if (ImGui::Button("更新", ImVec2(itemWidth, 0)))
            {

                int result = MessageBoxW(NULL,
                    L"設定を反映するにはアプリを再起動します。\n再起動しますか？",
                    L"再起動", MB_YESNO | MB_ICONQUESTION);

                if (result == IDYES)
                {
					useServer = tmpUseServer;
					autoStartModules = tmpAutoStartModules;
                    serverPort = tmpServerPort;
                    serverIP = tmpServerIP;
                    stopThread = true;
                    if (updateThread.joinable())
                        updateThread.join();

                    glDeleteTextures(1, &originalTexID);
                    glDeleteTextures(1, &realtimeTexID);
                    glDeleteTextures(1, &diffTexID);

                    SaveAppSettings();
                    ImGui::SaveIniSettingsToDisk(imguiIniPath.c_str());
                    ImGui_ImplOpenGL3_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext();
                    glfwDestroyWindow(window);
                    glfwTerminate();

                    running.store(false);
                    if (serverThread.joinable())
                    {
                        serverThread.join();
                    }
                    // 自分自身を再起動
                    char exePath[MAX_PATH];
                    GetModuleFileNameA(NULL, exePath, MAX_PATH);
                    ShellExecuteA(NULL, "open", exePath, NULL, NULL, SW_SHOWDEFAULT);

                    exit(0);
                }
            }


			ImGui::End();


        }

        std::unique_lock<std::mutex> lock(imgMutex);
        if (cv_newFrame.wait_for(lock, std::chrono::milliseconds(1), [&]
                                 { return newFrameReady; }))
        {
            if (!realtimeImg.empty() && realtimeImg.cols == width && realtimeImg.rows == height)
            {
                GLenum format = (realtimeImg.channels() == 3) ? GL_BGR : GL_BGRA;
                glBindTexture(GL_TEXTURE_2D, realtimeTexID);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, realtimeImg.cols, realtimeImg.rows, format, GL_UNSIGNED_BYTE, realtimeImg.data);
                glBindTexture(GL_TEXTURE_2D, 0);
				realtimeBase64 = MatToBase64(realtimeImg);
            }
            if (!emptyDiff.empty() && emptyDiff.cols == width && emptyDiff.rows == height)
            {
                glBindTexture(GL_TEXTURE_2D, diffTexID);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, emptyDiff.cols, emptyDiff.rows, GL_BGRA, GL_UNSIGNED_BYTE, emptyDiff.data);
                glBindTexture(GL_TEXTURE_2D, 0);
				diffBase64 = MatToBase64(emptyDiff);
            }
            newFrameReady = false;
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    stopThread = true;
    if (updateThread.joinable())
        updateThread.join();

    glDeleteTextures(1, &originalTexID);
    glDeleteTextures(1, &realtimeTexID);
    glDeleteTextures(1, &diffTexID);

    SaveAppSettings();
    ImGui::SaveIniSettingsToDisk(imguiIniPath.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    running.store(false);
    if (serverThread.joinable())
    {
        serverThread.join();
    }

    return 0;
}
