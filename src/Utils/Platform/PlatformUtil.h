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

    // --- 4. ファイルダイアログ (スタブ/実装) ---
#ifdef __EMSCRIPTEN__
    EM_ASYNC_JS(void, waitForFileSelect, (), {
        await new Promise((resolve) => {
            var input = document.createElement('input');
            input.type = 'file';
            input.accept = '.png,.jpg,.jpeg';
            input.onchange = async function(e) {
                var file = e.target.files[0];
                if (!file) { resolve(); return; }
                var data = await file.arrayBuffer();
                FS.writeFile('/tmp/selected_file', new Uint8Array(data));
                resolve();
            };
            input.oncancel = function() { resolve(); };
            input.click();
        });
    });
#endif

    inline std::string OpenFileDialog(void* nativeWindow, const char* filter) {
#if defined(_WIN32)
        char buf[512] = {};
        OPENFILENAMEA ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = (HWND)nativeWindow;
        ofn.lpstrFile = buf;
        ofn.nMaxFile = sizeof(buf);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        return GetOpenFileNameA(&ofn) ? std::string(buf) : "";
#elif defined(__EMSCRIPTEN__)
        waitForFileSelect();
        FILE* f = fopen("/tmp/selected_file", "rb");
        if (!f) return "";
        fclose(f);
        return "/tmp/selected_file";
#else
        return "";
#endif
    }
}