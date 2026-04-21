#include "Platform/PlatformUtil.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

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

namespace PlatformUtil
{
    std::string OpenFileDialog(void* nativeWindow, const char* filter) {
#if defined(_WIN32)
        char buf[260] = {};
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