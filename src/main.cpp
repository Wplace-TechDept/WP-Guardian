#define NOMINMAX // min/maxマクロの競合を回避
// --- Windows / Winsock ---
#include <winsock2.h>   // 先に読む
#include <windows.h>    // Windows API
// --- OpenGL / GLFW ---
#include <GL/glew.h>
#include <GLFW/glfw3.h>
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
#include "VersionManager/VersionManager.h"
#include "Utils/DiffPattern.h"
#include "ImageProcessing/ImageProcessing.h"
#include "ImageViewer/ImageViewer.h"
#include "Settings/Core/Settings.h"
#include "Settings/Core/SettingsIO.h"
#include "Settings/Core/SettingsJSON.h"
#include "Settings/UI/AppSettingsWindow.h"
#include "Settings/UI/UISettingsWindow.h"
#include "Utils/Platform/PathUtil.h"
#include "Utils/Platform/PlatformUtil.h"
#include "AppState.h"

#ifdef ENABLE_MODULE_SYSTEM
#include "Module/ModuleSystem.h"
#endif

using json = nlohmann::json;

// アプリバージョン
const std::string APP_VERSION = "v3.0.2"; //バージョン更新時にかならず書き換える
static Settings::WindowStates windowStates{
    true,  // showOriginal
    true,  // showRealtime
    true,  // showDiff
    true,  // showSettings
    true,  // showInfo
	false   // showUISettings
};
static Settings::AppSettings appSettings{
    1818,               // tile_x
    806,                // tile_y
    989,                // pixel_x
    358,                // pixel_y
    4.0f,               // updateSpeed
    DiffPattern{ DiffPatternType::ColorMask, ColorRGBA(0, 1.0f, 0, 1.0f) }, // diffPattern
    PlatformUtil::Path::GetExeDir()+"assets\\template.png" // imgPath
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
            PlatformUtil::OpenWebBrowser(updateCache.updateURL);
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

void SetAppIcon(GLFWwindow* window, const std::string& iconPath) {
    // OpenCVを使って画像を読み込む（プロジェクトの既存構成を活かす）
    cv::Mat img = cv::imread(iconPath, cv::IMREAD_UNCHANGED);
    if (img.empty()) return;

    // RGBAに変換
    cv::Mat rgba;
    cv::cvtColor(img, rgba, cv::COLOR_BGR2RGBA);

    GLFWimage images[1];
    images[0].width = rgba.cols;
    images[0].height = rgba.rows;
    images[0].pixels = rgba.data;

    glfwSetWindowIcon(window, 1, images);
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

int main()
{
	SetupConsoleIfDebug();// デバッグ時にコンソールを開く

    // 実行ファイルのディレクトリを取得し、INIファイルの絶対パスを作成
	std::string appDir = PlatformUtil::Path::GetExeDir();
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
    std::string iconPath = PlatformUtil::Path::GetExeDir() + "assets/app-icon.png";
    SetAppIcon(window, iconPath);
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
        std::cerr << "フォント読み込み失敗\n";
    if (!bigFont)
        std::cerr << "フォント読み込み失敗\n";
	// -----------------


    // アプリ起動時に設定を読み込む
    Settings::IO::Load(windowStates, appSettings, updateCache, uiSettings);

    AppState state;

    state.appSettings = &appSettings;

#ifdef ENABLE_MODULE_SYSTEM
    ModuleSystem moduleSystem(state);
    moduleSystem.start();
#endif

    state.originalImg = cv::imread(appSettings.imgPath, cv::IMREAD_UNCHANGED);
    if (state.originalImg.empty())
    {
        state.originalImg = cv::Mat(1, 1, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    }
    ensureBGRA(state.originalImg);


    state.width = state.originalImg.cols;
    state.height = state.originalImg.rows;

    state.realtimeImg = cv::Mat::zeros(state.height, state.width, CV_8UC4);
    state.diffImg = cv::Mat::zeros(state.height, state.width, CV_8UC4);

    GLuint originalTexID = matToTexture(state.originalImg);
    GLuint realtimeTexID = createEmptyTexture(state.width, state.height);
    GLuint diffTexID = createEmptyTexture(state.width, state.height);

	ImageProcessing imgProc;
	imgProc.SetTileParams(appSettings.tile_x, appSettings.tile_y, appSettings.pixel_x, appSettings.pixel_y);
	imgProc.SetDiffPattern(appSettings.diffPattern);
	imgProc.SetUpdateSpeed(appSettings.updateSpeed);
	imgProc.SetOriginalImage(state.originalImg);

	state.totalPixels = 0;
    state.diffPixels = 0;
    state.diffPercent = 0;

	Settings::AppSettingsWindow settingsWindow(appSettings,window,bigFont);
	Settings::UISettingsWindow uiSettingsWindow;

    state.getTempSettings = [&]() { return settingsWindow.GetTempSettings(); };
    state.setTempSettings = [&](const Settings::AppSettings& s) { settingsWindow.SetTempSettings(s); };

    state.update = false;

    ImageViewer OriginalViewer(state.width, state.height);
	ImageViewer RealtimeViewer(state.width, state.height);
	ImageViewer DiffViewer(state.width, state.height);

    imgProc.start();

    auto cleanup = [&]() {
        imgProc.stop();
#ifdef ENABLE_MODULE_SYSTEM
        moduleSystem.stop();
#endif
        glDeleteTextures(1, &originalTexID);
        glDeleteTextures(1, &realtimeTexID);
        glDeleteTextures(1, &diffTexID);
        Settings::IO::Save(windowStates, appSettings, updateCache, uiSettings);
        ImGui::SaveIniSettingsToDisk(imguiIniPath.c_str());
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        };

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
				ImGui::MenuItem("UI設定", nullptr, &windowStates.showUISettings);
#ifdef ENABLE_MODULE_SYSTEM
                moduleSystem.drawWindowMenuItems();
#endif
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("詳細"))
            {
                ImGui::MenuItem("アップデートを確認", nullptr, &showVersion);
#ifdef ENABLE_MODULE_SYSTEM
                moduleSystem.drawModuleManagerMenuItems();
#endif
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        showVersionWindow();

#ifdef ENABLE_MODULE_SYSTEM
        moduleSystem.update();
        moduleSystem.drawUI();
#endif

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
#ifdef ENABLE_MODULE_SYSTEM
        if (moduleSystem.needsRestart()) {
            int answer = MessageBoxW(NULL,
                L"設定を反映するにはアプリを再起動します。\n再起動しますか？",
                L"再起動", MB_YESNO | MB_ICONQUESTION);

            if (answer == IDYES)
            {
                std::string exePath = PathUtil::GetExePath();
                cleanup();
                PlatformUtil::RestartApp(exePath);
            }
        }
#endif
        if (settingsWindow.Draw(windowStates.showSettings)|| state.update) {
            state.update = false;

            Settings::AppSettings newSettings = settingsWindow.GetTempSettings();
            appSettings.imgPath = newSettings.imgPath;

            if (!appSettings.imgPath.empty())
            {
                cv::Mat newImg = cv::imread(appSettings.imgPath, cv::IMREAD_UNCHANGED);
                if (!newImg.empty())
                {
                    state.originalImg = newImg.clone();
                    ensureBGRA(state.originalImg);
                    imgProc.SetOriginalImage(state.originalImg);
                    state.width = state.originalImg.cols;
                    state.height = state.originalImg.rows;

                    state.originalBase64.data.clear();
                    state.originalBase64.hasData = false;

                    OriginalViewer.SetImageSize(state.width, state.height);
                    RealtimeViewer.SetImageSize(state.width, state.height);
                    DiffViewer.SetImageSize(state.width, state.height);
                    OriginalViewer.ResetView();
                    RealtimeViewer.ResetView();
                    DiffViewer.ResetView();
                    // original
                    glDeleteTextures(1, &originalTexID);
                    originalTexID = matToTexture(newImg);

                    // realtime / diff
                    glDeleteTextures(1, &realtimeTexID);
                    glDeleteTextures(1, &diffTexID);

                    realtimeTexID = createEmptyTexture(state.width, state.height);
                    diffTexID = createEmptyTexture(state.width, state.height);
					std::cout << "画像を更新しました: " << appSettings.imgPath << " (" << state.width << "x" << state.height << ")\n";
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
                state.diffPercent = output.diffPercent;
                state.totalPixels = output.totalOpaquePixels;
                state.diffPixels = output.diffPixels;
                // テクスチャを更新
                if (!SafeUploadTexture(realtimeTexID, state.width, state.height, output.realtimeImg)) {
                    std::cerr << "Failed to upload realtime texture\n";
                }
                state.realtimeImg = output.realtimeImg.clone();
                if (!SafeUploadTexture(diffTexID, state.width, state.height, output.diffImg)) {
                    std::cerr << "Failed to upload diff texture\n";
                }
                state.diffImg = output.diffImg.clone();

                state.realtimeBase64.data.clear();
                state.realtimeBase64.hasData = false;
                state.diffBase64.data.clear();
                state.diffBase64.hasData = false;
            }
        }

        if (windowStates.showInfo)
        {
            ImGui::Begin("情報", &windowStates.showInfo);
            ImGui::PushFont(bigFont);
            {
                ImGui::Text("差分率: %.2f%%", state.diffPercent);
                ImGui::Text("%d / %d", state.diffPixels, state.totalPixels);
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

    cleanup();
    return 0;
}
