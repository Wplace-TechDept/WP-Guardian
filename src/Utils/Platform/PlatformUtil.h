#pragma once
#include <string>
#include <iostream>

// --- Windows 固有のインクルード ---
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
    #include <windows.h>
    #include <shellapi.h>
    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#endif

// --- WASM 固有のインクルード ---
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

namespace PlatformUtil
{
    // --- 【内部用ヘルパー】 Windowsでのみ使用 ---
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
    inline std::wstring Utf8ToWide(const std::string& utf8Str) {
        if (utf8Str.empty()) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
#endif
    // --- 1. デバッグコンソール ---
    inline void SetupDebugConsole() {
#if defined(_WIN32) && defined(_DEBUG)
        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
            std::cout << u8"[Debug] Console Initialized." << std::endl;
        }
#endif
        // WASM/Linux は標準出力がそのまま使えるため何もしない
    }

    // --- 2. ブラウザ / URL オープン ---
    inline void OpenWebBrowser(const std::string& url) {
#if defined(_WIN32)
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__EMSCRIPTEN__)
        std::string script = "window.open('" + url + "', '_blank');";
        emscripten_run_script(script.c_str());
#endif
    }

    // --- 3. アプリ再起動 ---
    inline void RestartApp(const std::string& path) {
#if defined(_WIN32)
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        exit(0);
#elif defined(__EMSCRIPTEN__)
        emscripten_run_script("window.location.reload();");
#endif
    }

    std::string OpenFileDialog(void* nativeWindow, const char* filter);

    inline bool ShowYesNoDialog(const std::string& title, const std::string& message) {
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
        // Windows: Wide版APIに変換して渡す（文字化け防止）
        int result = MessageBoxW(
            NULL, 
            Utf8ToWide(message).c_str(), 
            Utf8ToWide(title).c_str(), 
            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2
        );
        return (result == IDYES);

#elif defined(__EMSCRIPTEN__)
        // WASM: ブラウザの標準confirmを使用
        #include <emscripten.h>
        return emscripten_run_script_int(("confirm('" + message + "')").c_str());

#else
        // Linux等: とりあえずコンソール出力
        std::cout << "\n[" << title << "]\n" << message << " (y/n): ";
        char response;
        std::cin >> response;
        return (response == 'y' || response == 'Y');
#endif
    }
}