#pragma once
#include <string>

namespace PlatformUtil
{
    namespace Path
    {
        inline std::string GetFontPath()
        {
#ifdef __EMSCRIPTEN__
            // WASM: Emscriptenの仮想ディレクトリ
            return "/assets/fonts/meiryo.ttc";
#elif defined(_WIN32)
            // Windows: システムフォントパス
            return "C:/Windows/Fonts/meiryo.ttc";
#else
            // Linux等
            return ""; 
#endif
        }
    }
}