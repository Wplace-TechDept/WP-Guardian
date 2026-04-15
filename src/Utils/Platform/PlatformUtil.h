#pragma once
#include <string>

#ifndef __EMSCRIPTEN__
#include <windows.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace PlatformUtil
{
    inline std::string OpenFileDialog(void* nativeWindow, const char* filter) {
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
    }
    inline void OpenWebBrowser(const std::string& url){
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    inline void RestartApp(const std::string& exePath) {
        ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        exit(0);
    }
    inline void SetupDebugConsole() {
#if defined(_WIN32) && defined(_DEBUG)
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
}

#else
#include <emscripten.h>

EM_ASYNC_JS(void, waitForFileSelect, (), {
    await new Promise((resolve) => {
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = '.png,.jpg,.jpeg';
        input.onchange = async function(e) {
            var file = e.target.files[0];
            var data = await file.arrayBuffer();
            FS.writeFile('/tmp/selected_file', new Uint8Array(data));
            resolve();
        };
        input.oncancel = function() { resolve(); };
        input.click();
    });
});

namespace PlatformUtil
{
    inline std::string OpenFileDialog(void* nativeWindow, const char* filter) {
        waitForFileSelect();
        FILE* f = fopen("/tmp/selected_file", "rb");
        if (!f) return "";
        fclose(f);
        return "/tmp/selected_file";
    }
    inline void OpenWebBrowser(const std::string& url) {
        std::string script = "window.open('" + url + "', '_blank');";
        emscripten_run_script(script.c_str());
    }
    inline void SetupDebugConsole() {
    }
}
#endif