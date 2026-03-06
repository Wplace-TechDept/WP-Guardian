#pragma once
#include <string>

namespace ModuleSettings {
    struct Settings {
        bool autoStartModules = false;
        bool useServer = false;
        std::string serverIP = "127.0.0.1";
        int serverPort = 9002;
    };
    struct ModuleWindowStates {
        bool showWindowManager = false;
        bool showModuleSettings = false;
    };
}