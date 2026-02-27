#pragma once
#include <string>

namespace ModuleSettings {
    struct Settings {
        bool autoStartModules;
        bool useServer;
        std::string serverIP;
        int serverPort;
    };
    struct ModuleWindowStates {
        bool showWindowManager = false;
        bool showModuleSettings = false;
    };
}