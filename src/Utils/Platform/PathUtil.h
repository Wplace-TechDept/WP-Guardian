#pragma once
#include <string>

#ifndef __EMSCRIPTEN__
#include <windows.h>

namespace PathUtil
{
    inline std::string GetExeDir()
    {
        char pathBuf[MAX_PATH];
        GetModuleFileNameA(NULL, pathBuf, MAX_PATH);
        std::string fullPath(pathBuf);
        size_t lastSlash = fullPath.find_last_of("\\/");
        return fullPath.substr(0, lastSlash + 1);
    }
    inline std::string GetExePath()
    {
        char buf[MAX_PATH];
        GetModuleFileNameA(NULL, buf, MAX_PATH);
        return std::string(buf);
    }
}

#else
// WASM版: ファイルシステムの概念がないためスタブ
namespace PathUtil
{
    inline std::string GetExeDir() { return ""; }
    inline std::string GetExePath() { return ""; }
}
#endif