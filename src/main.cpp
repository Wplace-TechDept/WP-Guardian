#define NOMINMAX // min/maxマクロの競合を回避
// --- Windows / Winsock ---
#include <winsock2.h>   // 先に読む
#include <windows.h>    // Windows API
// --- OpenGL / GLFW ---
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
// --- OpenCV ---
#include <opencv2/opencv.hpp>
// --- ImGui ---
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// --- 標準ライブラリ ---
#include <cstdlib>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <vector>
#include <cstring>
// --- 自作ヘッダ ---
#include "ImWS.h"
#include "WSVariable.h"
#include "Base64Codec/Base64Codec.h"
#include "VersionManager/VersionManager.h"
#include "ModuleManager/ModuleManager.h"
#include "Utils/DiffPattern.h"
#include "ImageProcessing/ImageProcessing.h"
#include "Server/Server.h"
#include "ImageViewer/ImageViewer.h"
#include "Settings/Core/Settings.h"
#include "Settings/Core/SettingsIO.h"
#include "Settings/Core/SettingsJSON.h"
#include "Settings/UI/AppSettingsWindow.h"
#include "Settings/UI/ModuleSettingsWindow.h"
#include "Settings/UI/UISettingsWindow.h"
#include "PathUtil.h"


using json = nlohmann::json;

// アプリバージョン
const std::string APP_VERSION = "v3.0.2"; //バージョン更新時にかならず書き換える
static Settings::WindowStates windowStates{
    true,  // showOriginal
    true,  // showRealtime
    true,  // showDiff
    true,  // showSettings
    true,  // showInfo
    false, // showWindowManager
	false, // showModuleSeting
	false   // showUISettings
};
static Settings::AppSettings appSettings{
    1818,               // tile_x
    806,                // tile_y
    989,                // pixel_x
    358,                // pixel_y
    4.0f,               // updateSpeed
    DiffPattern{ DiffPatternType::ColorMask, ColorRGBA(0, 1.0f, 0, 1.0f) }, // diffPattern
	GetExeDir()+"assets\\template.png" // imgPath
};
static Settings::ModuleSettings moduleSettings{
    false,              // autoStartModules
    false,               // useServer
	"127.0.0.1",        // serverIP
	9002                // serverPort
};
static Settings::UpdateCache updateCache{
    false,              // foundUpdate
    "",                 // latestVersion
    "",                 // updateURL
    true                // showUpdateMessage
};
static Settings::UISettings uiSettings{
    true,               // isLinkZoom
    1.0f,               // UIScale
    8.0f,               // WindowRounding
    3.0f,               // FrameRounding
};
// ----------------------------

// バージョン
VersionManager vm("Kikumon-Tech", "WP-Guardian", APP_VERSION);

bool showVersion;
void showVersionWindow() {

    static bool updateCheckedThisSession = false;

    if (!updateCheckedThisSession) {
        if (!updateCache.foundUpdate) {
            VersionManager::UpdateResult result = vm.checkUpdate();
            updateCache.foundUpdate = result.hasUpdate;
            updateCache.latestVersion = result.latestVersion;
            updateCache.updateURL = result.updateURL;
        }
        updateCheckedThisSession = true;
		std::cout << "Update check complete. Found update: " << (updateCache.foundUpdate ? "Yes" : "No") << std::endl;
        showVersion = updateCache.foundUpdate;
    }

	if (!showVersion) return;

    static bool opened = false;
    if (!opened) {
        ImGui::OpenPopup("バージョン");
        opened = true;
    }
    if (ImGui::BeginPopupModal("バージョン", &showVersion, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (updateCache.foundUpdate) {
            ImGui::Text("新しいバージョンがあります！ 最新バージョン: %s", updateCache.latestVersion.c_str());
        }
        else {
            ImGui::Text("既に最新バージョンです！ %s", APP_VERSION.c_str());
        }

        if (ImGui::Button("アップデートページを開く")) {
            ShellExecuteA(NULL, "open", updateCache.updateURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }

        bool hideUpdateMessage = !updateCache.showUpdateMessage;
        if (ImGui::Checkbox("このメッセージを再度表示しない", &hideUpdateMessage))
            updateCache.showUpdateMessage = !hideUpdateMessage;

        if (ImGui::Button("閉じる")) {
            showVersion = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (!showVersion) {
        opened = false;
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

ModuleManager mm("assets/moduleList.json");

std::string MatToBase64(const cv::Mat& mat)
{
    // 元は BGRA
    CV_Assert(mat.type() == CV_8UC4);
    CV_Assert(mat.isContinuous());

    cv::Mat rgbaMat;
    cv::cvtColor(mat, rgbaMat, cv::COLOR_BGRA2RGBA); // B G R A → R G B A

    const uint8_t* data = rgbaMat.data;
    size_t dataSize = rgbaMat.total() * rgbaMat.elemSize();

    static Base64Codec codec;
    return codec.encode(data, dataSize);
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

void setWindowIconFromExe(GLFWwindow* window)
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

void ensureBGRA(cv::Mat& img)
{
    if (img.empty())
        return;
    if (img.channels() == 3)
        cv::cvtColor(img, img, cv::COLOR_BGR2BGRA);
}

GLuint matToTexture(const cv::Mat& mat)
{
    if (mat.empty())
        return 0;
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    GLenum format = (mat.channels() == 3) ? GL_BGR : GL_BGRA;
    GLint internalFormat = (mat.channels() == 3) ? GL_RGB8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, mat.cols, mat.rows, 0, format, GL_UNSIGNED_BYTE, mat.data);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}
GLuint createEmptyTexture(int w, int h)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // ★ 修正：w * h 分の透明なピクセルデータ（0）を用意する
    // BGRAなので、全バイト0なら「透明な黒」になります
    std::vector<uint32_t> emptyPixels(w * h, 0);

    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RGBA8,
        w, h,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        emptyPixels.data() // ★ nullptr ではなく作成したデータを渡す
    );

    float borderColor[] = { 0.f, 0.f, 0.f, 0.f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

static bool SafeUploadTexture(GLuint texID, int w, int h, const cv::Mat& mat)
{
    if (texID == 0) {
        std::cerr << "SafeUploadTexture: invalid texID\n";
        return false;
    }
    if (w <= 0 || h <= 0) {
        std::cerr << "SafeUploadTexture: invalid size " << w << "x" << h << "\n";
        return false;
    }
    if (mat.empty()) {
        std::cerr << "SafeUploadTexture: mat is empty\n";
        return false;
    }
    if (mat.cols != w || mat.rows != h) {
        std::cerr << "SafeUploadTexture: mat size mismatch: mat=" << mat.cols << "x" << mat.rows << " expected=" << w << "x" << h << "\n";
        return false;
    }
    if (mat.type() != CV_8UC4 && mat.type() != CV_8UC3) {
        std::cerr << "SafeUploadTexture: unsupported mat.type()=" << mat.type() << "\n";
        return false;
    }

    // 連続メモリでない場合はクローンして連続化
    cv::Mat src = mat.isContinuous() ? mat : mat.clone();

    // GL に渡す直前に深コピーして、producer スレッドが変更しても安全にする
    size_t bytes = static_cast<size_t>(src.total() * src.elemSize());
    std::vector<unsigned char> buffer(bytes);
    std::memcpy(buffer.data(), src.data, bytes);

    glBindTexture(GL_TEXTURE_2D, texID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLenum format = (src.channels() == 3) ? GL_BGR : GL_BGRA;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, buffer.data());
    GLenum err = glGetError();
    glBindTexture(GL_TEXTURE_2D, 0);

    if (err != GL_NO_ERROR) {
        std::cerr << "SafeUploadTexture: glTexSubImage2D GL error: 0x" << std::hex << err << std::dec << "\n";
        return false;
    }
    return true;
}

struct Base64ImageCache {
    std::string data = "";  // Base64文字列
    bool hasData = false;   // キャッシュが有効か
};

int main()
{
	SetupConsoleIfDebug();// デバッグ時にコンソールを開く

    // 実行ファイルのディレクトリを取得し、INIファイルの絶対パスを作成
	std::string appDir = GetExeDir();
    std::string imguiIniPath = appDir + "imgui.ini";
	std::cout << "INIファイルのパス: " << imguiIniPath << std::endl;

	// ---GLFW初期化---
    if (!glfwInit())
        return -1;
    GLFWwindow* window = glfwCreateWindow(1200, 800, "WP-Guardian", nullptr, nullptr);
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
    // --------------
    
	// ---ImGui初期化---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = imguiIniPath.c_str();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);

    ImFontConfig font_cfg{};
    font_cfg.FontDataOwnedByAtlas = true;

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/meiryo.ttc", 18.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
    ImFont* bigFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/meiryo.ttc", 25.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese());
    if (!font)
        std::cerr << "フォント読み込み失敗！\n";
    if (!bigFont)
        std::cerr << "フォント読み込み失敗！\n";
	// -----------------


    // アプリ起動時に設定を読み込む
    Settings::IO::Load(windowStates, appSettings, moduleSettings, updateCache, uiSettings);

	// WebSocketサーバー起動処理
	Server server;
    auto bind_ws = [&](auto& ws) {
        server.add_open_handler(
            [&](websocketpp::connection_hdl hdl) { ws.on_open(hdl); }
        );
        server.add_close_handler(
            [&](websocketpp::connection_hdl hdl) { ws.on_close(hdl); }
        );
        server.add_message_handler(
            [&](websocketpp::connection_hdl hdl, server::message_ptr msg) {
                ws.on_message(hdl, msg);
            }
        );
    };

    ImWS imws;
    imws.send = [&](auto hdl, const std::string& msg) {
        server.send(hdl, msg);
        };

    WSVariable wsVar;
    wsVar.send = [&](auto hdl, const std::string& msg) {
        server.send(hdl, msg);
        };

    bind_ws(imws);
    bind_ws(wsVar);

    // useServerがtrueの場合にのみ、ポートの空き状況をチェック
    if (moduleSettings.useServer) {
        // 1. ポートが使用可能かメインスレッドで先に確認する
        bool isPortAvailable = can_bind(moduleSettings.serverIP, moduleSettings.serverPort);

        if (isPortAvailable) {
            // ポートが空いているので、サーバーを別スレッドで起動
			server.start(moduleSettings.serverIP, moduleSettings.serverPort);
        }
        else {
            // 2. ポートが使用中のため、サーバーは起動せず、フラグを更新
            std::cerr << "ポートがすでに使用されています: " << moduleSettings.serverPort << "。サーバーを起動しません。" << std::endl;
            // サーバーが起動しないため、関連するモジュールの自動起動も無効化する
            moduleSettings.useServer = false;
            moduleSettings.autoStartModules = false;
        }
    }

    // 3. 最終的に確定した autoStartModules の値に基づいてモジュールを起動
    if (moduleSettings.autoStartModules) {
        mm.LaunchModules();
    }
    cv::Mat originalImg = cv::imread(appSettings.imgPath, cv::IMREAD_UNCHANGED);
    if (originalImg.empty())
    {
        originalImg = cv::Mat(1, 1, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    }
    ensureBGRA(originalImg);
    int width = originalImg.cols;
    int height = originalImg.rows;

    cv::Mat realtimeImg = cv::Mat::zeros(height, width, CV_8UC4);
	cv::Mat diffImg = cv::Mat::zeros(height, width, CV_8UC4);

    GLuint originalTexID = matToTexture(originalImg);
    GLuint realtimeTexID = createEmptyTexture(width, height);
    GLuint diffTexID = createEmptyTexture(width, height);

    Base64ImageCache originalBase64;
    Base64ImageCache realtimeBase64;
    Base64ImageCache diffBase64;

	ImageProcessing imgProc;
	imgProc.SetTileParams(appSettings.tile_x, appSettings.tile_y, appSettings.pixel_x, appSettings.pixel_y);
	imgProc.SetDiffPattern(appSettings.diffPattern);
	imgProc.SetUpdateSpeed(appSettings.updateSpeed);
	imgProc.SetOriginalImage(originalImg);

	int totalPixels = 0;
	int diffPixels = 0;
	double diffPercent = 0;

	Settings::AppSettingsWindow settingsWindow(appSettings,window,bigFont);
	Settings::ModuleSettingsWindow moduleSettingsWindow(moduleSettings);
	Settings::UISettingsWindow uiSettingsWindow;

    bool update = false;

    ImageViewer OriginalViewer(width, height);
	ImageViewer RealtimeViewer(width, height);
	ImageViewer DiffViewer(width, height);

    imgProc.start();

    wsVar.register_setter("update", [&](const json& value) {
        update = value.get<bool>();
    });
    wsVar.register_getter("settings.temp", [&]() -> json {
        json j;
        Settings::JSON::ToJSON(j, settingsWindow.GetTempSettings());
        return j["AppSettings"];
    });
    wsVar.register_setter("settings.temp",[&](const json& value){
        auto& temp = settingsWindow.GetTempSettings();

        json wrapper;
        wrapper["AppSettings"] = value;

        Settings::JSON::FromJSON(wrapper, temp);
    });

    wsVar.register_getter("settings", [&]() -> json {
        json j;
        Settings::JSON::ToJSON(j, appSettings);
        return j["AppSettings"];
    });

    wsVar.register_getter("info", [&]() -> json {
        json j;
        j["totalPixels"] = totalPixels;
        j["diffPixels"] = diffPixels;
        j["diffPercent"] = diffPercent;
        return j;
    });

    wsVar.register_getter("image.size", [&]() -> json {
        return{
            {"width", width},
            {"height", height}
        };
    });

    wsVar.register_getter("image.original", [&]() -> json {
        if (!originalBase64.hasData) {
            originalBase64.data = MatToBase64(originalImg);
            originalBase64.hasData = true;
        }
        return{
            {"width", originalImg.cols},
            {"height", originalImg.rows},
            {"data", originalBase64.data}
        };
    });
    wsVar.register_getter("image.realtime", [&]() -> json {
        if (!realtimeBase64.hasData) {
            realtimeBase64.data = MatToBase64(realtimeImg);
            realtimeBase64.hasData = true;
        }
        return{
            {"width", realtimeImg.cols},
            {"height", realtimeImg.rows},
            {"data", realtimeBase64.data}
        };
    });
    wsVar.register_getter("image.diff", [&]() -> json {
        if (!diffBase64.hasData) {
            diffBase64.data = MatToBase64(diffImg);
            diffBase64.hasData = true;
        }
        return{
            {"width", diffImg.cols},
            {"height", diffImg.rows},
            {"data", diffBase64.data}
        };
    });

    glfwSetWindowCloseCallback(window, [](GLFWwindow* win)
        {
            glfwSetWindowShouldClose(win, GLFW_TRUE);
        });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::DockSpaceOverViewport(viewport->ID, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("ウィンドウ"))
            {
                ImGui::MenuItem("オリジナル画像", nullptr, &windowStates.showOriginal);
                ImGui::MenuItem("リアルタイム画像", nullptr, &windowStates.showRealtime);
                ImGui::MenuItem("差分画像", nullptr, &windowStates.showDiff);
                ImGui::MenuItem("設定", nullptr, &windowStates.showSettings);
                ImGui::MenuItem("情報", nullptr, &windowStates.showInfo);
                ImGui::MenuItem("ウィンドウ管理", nullptr, &windowStates.showWindowManager);
                ImGui::MenuItem("モジュール設定", nullptr, &windowStates.showModuleSetting);
				ImGui::MenuItem("UI設定", nullptr, &windowStates.showUISettings);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("詳細"))
            {
                ImGui::MenuItem("アップデートを確認", nullptr, &showVersion);
                if (ImGui::MenuItem("モジュール管理")) {
                    mm.SetShow(true);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        imws.update();
        imws.drawWindowManager(windowStates.showWindowManager);
        wsVar.update();
        mm.ShowWindow();
        showVersionWindow();
		uiSettingsWindow.Draw(windowStates.showUISettings, uiSettings);


        if (windowStates.showOriginal)
        {
            ImGui::Begin("オリジナル画像", &windowStates.showOriginal);
			OriginalViewer.BeginFrame();
			OriginalViewer.Update(window);
            if (uiSettings.isLinkZoom) {
                ImageViewEvent OriginalEvent = OriginalViewer.GetEvent();
                if (OriginalEvent.zoomed) {
                    RealtimeViewer.ApplyZoom(OriginalEvent.zoomOrigin, OriginalEvent.zoomFactor);
                    DiffViewer.ApplyZoom(OriginalEvent.zoomOrigin, OriginalEvent.zoomFactor);
				}
                if (OriginalEvent.panned) {
                    RealtimeViewer.ApplyPan(OriginalEvent.panDelta);
                    DiffViewer.ApplyPan(OriginalEvent.panDelta);
				}
                if (OriginalEvent.reset) {
                    RealtimeViewer.ResetView();
                    DiffViewer.ResetView();
                }
            }
			OriginalViewer.Draw((ImTextureID)(intptr_t)originalTexID);
            ImGui::End();
        }

        if (windowStates.showRealtime)
        {
            ImGui::Begin("リアルタイム画像", &windowStates.showRealtime);
			RealtimeViewer.BeginFrame();
			RealtimeViewer.Update(window);
            if (uiSettings.isLinkZoom) {
                ImageViewEvent RealtimeEvent = RealtimeViewer.GetEvent();
                if (RealtimeEvent.zoomed) {
                    OriginalViewer.ApplyZoom(RealtimeEvent.zoomOrigin, RealtimeEvent.zoomFactor);
                    DiffViewer.ApplyZoom(RealtimeEvent.zoomOrigin, RealtimeEvent.zoomFactor);
                }
                if (RealtimeEvent.panned) {
                    OriginalViewer.ApplyPan(RealtimeEvent.panDelta);
                    DiffViewer.ApplyPan(RealtimeEvent.panDelta);
                }
                if (RealtimeEvent.reset) {
                    OriginalViewer.ResetView();
                    DiffViewer.ResetView();
                }
			}
			RealtimeViewer.Draw((ImTextureID)(intptr_t)realtimeTexID);
            ImGui::End();
        }

        if (windowStates.showDiff)
        {
            ImGui::Begin("差分画像", &windowStates.showDiff);
			DiffViewer.BeginFrame();
			DiffViewer.Update(window);
            if (uiSettings.isLinkZoom) {
                ImageViewEvent DiffEvent = DiffViewer.GetEvent();
                if (DiffEvent.zoomed) {
                    OriginalViewer.ApplyZoom(DiffEvent.zoomOrigin, DiffEvent.zoomFactor);
                    RealtimeViewer.ApplyZoom(DiffEvent.zoomOrigin, DiffEvent.zoomFactor);
                }
                if (DiffEvent.panned) {
                    OriginalViewer.ApplyPan(DiffEvent.panDelta);
                    RealtimeViewer.ApplyPan(DiffEvent.panDelta);
                }
                if (DiffEvent.reset) {
                    OriginalViewer.ResetView();
                    RealtimeViewer.ResetView();
                }
			}
			DiffViewer.Draw((ImTextureID)(intptr_t)diffTexID);
            ImGui::End();
        }

		auto result = moduleSettingsWindow.Draw(windowStates.showModuleSetting);
        if (result.openModuleManager) {
			mm.SetShow(true);
		}
        if (result.updated) {

            int result = MessageBoxW(NULL,
                L"設定を反映するにはアプリを再起動します。\n再起動しますか？",
                L"再起動", MB_YESNO | MB_ICONQUESTION);

            if (result == IDYES)
            {
				auto newSettings = moduleSettingsWindow.GetTempSettings();
                moduleSettings.useServer = newSettings.useServer;
                moduleSettings.autoStartModules = newSettings.autoStartModules;
                moduleSettings.serverPort = newSettings.serverPort;
                moduleSettings.serverIP = newSettings.serverIP;

                imgProc.stop();

                glDeleteTextures(1, &originalTexID);
                glDeleteTextures(1, &realtimeTexID);
                glDeleteTextures(1, &diffTexID);

				Settings::IO::Save(windowStates, appSettings, moduleSettings, updateCache, uiSettings);
                ImGui::SaveIniSettingsToDisk(imguiIniPath.c_str());
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                glfwDestroyWindow(window);
                glfwTerminate();

                server.stop();

                // 自分自身を再起動
                char exePath[MAX_PATH];
                GetModuleFileNameA(NULL, exePath, MAX_PATH);
                ShellExecuteA(NULL, "open", exePath, NULL, NULL, SW_SHOWDEFAULT);

                exit(0);
            }
        }

        if (settingsWindow.Draw(windowStates.showSettings)||update) {
            update = false;

            Settings::AppSettings newSettings = settingsWindow.GetTempSettings();
            appSettings.imgPath = newSettings.imgPath;

            if (!appSettings.imgPath.empty())
            {
                cv::Mat newImg = cv::imread(appSettings.imgPath, cv::IMREAD_UNCHANGED);
                if (!newImg.empty())
                {
                    originalImg = newImg.clone();
                    ensureBGRA(originalImg);
                    imgProc.SetOriginalImage(originalImg);
                    width = originalImg.cols;
                    height = originalImg.rows;

                    originalBase64.data.clear();
                    originalBase64.hasData = false;

                    OriginalViewer.SetImageSize(width, height);
                    RealtimeViewer.SetImageSize(width, height);
                    DiffViewer.SetImageSize(width, height);
                    OriginalViewer.ResetView();
                    RealtimeViewer.ResetView();
                    DiffViewer.ResetView();
                    // original
                    glDeleteTextures(1, &originalTexID);
                    originalTexID = matToTexture(newImg);

                    // realtime / diff
                    glDeleteTextures(1, &realtimeTexID);
                    glDeleteTextures(1, &diffTexID);

                    realtimeTexID = createEmptyTexture(width, height);
                    diffTexID = createEmptyTexture(width, height);
					std::cout << "画像を更新しました: " << appSettings.imgPath << " (" << width << "x" << height << ")\n";
                }
            }

			appSettings.tile_x = newSettings.tile_x;
			appSettings.tile_y = newSettings.tile_y;
			appSettings.pixel_x = newSettings.pixel_x;
			appSettings.pixel_y = newSettings.pixel_y;
            imgProc.SetTileParams(appSettings.tile_x, appSettings.tile_y, appSettings.pixel_x, appSettings.pixel_y);
            appSettings.updateSpeed = newSettings.updateSpeed;
            imgProc.SetUpdateSpeed(appSettings.updateSpeed);
            appSettings.diffPattern = newSettings.diffPattern;
            imgProc.SetDiffPattern(appSettings.diffPattern);

        }
        else {
            ImageProcessing::Output output;

            if (imgProc.fetchOutput(output)) {
                diffPercent = output.diffPercent;
                totalPixels = output.totalOpaquePixels;
                diffPixels = output.diffPixels;
                // テクスチャを更新
                if (!SafeUploadTexture(realtimeTexID, width, height, output.realtimeImg)) {
                    std::cerr << "Failed to upload realtime texture\n";
                }
                realtimeImg = output.realtimeImg.clone();
                if (!SafeUploadTexture(diffTexID, width, height, output.diffImg)) {
                    std::cerr << "Failed to upload diff texture\n";
                }
                diffImg = output.diffImg.clone();

                realtimeBase64.data.clear();
                realtimeBase64.hasData = false;
                diffBase64.data.clear();
                diffBase64.hasData = false;
            }
        }

        if (windowStates.showInfo)
        {
            ImGui::Begin("情報", &windowStates.showInfo);
            ImGui::PushFont(bigFont);
            {
                ImGui::Text("差分率: %.2f%%", diffPercent);
                ImGui::Text("%d / %d", diffPixels, totalPixels);
            }
            ImGui::PopFont();
            ImGui::End();
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
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

	imgProc.stop();

    glDeleteTextures(1, &originalTexID);
    glDeleteTextures(1, &realtimeTexID);
    glDeleteTextures(1, &diffTexID);

    Settings::IO::Save(windowStates, appSettings, moduleSettings, updateCache, uiSettings);


    ImGui::SaveIniSettingsToDisk(imguiIniPath.c_str());
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    server.stop();

    return 0;
}
