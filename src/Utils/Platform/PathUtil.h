#pragma once
#include <string>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
    #include <windows.h>
#endif

namespace PlatformUtil
{
    namespace Path
    {
        inline std::string GetExePath()
        {
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
            char buf[MAX_PATH];
            GetModuleFileNameA(NULL, buf, MAX_PATH);
            return std::string(buf);
#else
            // WASMやLinux（暫定）では空、または適切な実装を後で追加
            return "";
#endif
        }

        inline std::string GetExeDir()
        {
#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
            std::string fullPath = GetExePath();
            size_t lastSlash = fullPath.find_last_of("\\/");
            if (lastSlash == std::string::npos) return "";
            return fullPath.substr(0, lastSlash + 1);
#else
            return "";
#endif
        }
    }
}