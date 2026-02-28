#pragma once 
#include "ModuleSettings.h"
#include "imgui.h"

struct ModuleSettingsWindowResult {
	bool updated = false;       // 設定を更新した
	bool openModuleManager = false; // モジュール管理画面を開くボタン押下
};
namespace ModuleSettings {
    class ModuleSettingsWindow {
    public:
        ModuleSettingsWindow(Settings settings);
        ~ModuleSettingsWindow();
        ModuleSettingsWindowResult Draw(bool& isShow);
        const Settings& GetTempSettings() const { return tempSettings; }
        Settings& GetTempSettings() { return tempSettings; }
    private:
        Settings tempSettings{};
    };
}
