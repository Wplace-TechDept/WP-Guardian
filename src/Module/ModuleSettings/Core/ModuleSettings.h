#pragma once
#include <string>

namespace ModuleSettings {
    struct ModuleSettings {
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