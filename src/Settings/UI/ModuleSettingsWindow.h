#pragma once 
#include "Settings.h"
#include "imgui.h"

struct ModuleSettingsWindowResult {
	bool updated = false;       // 設定を更新した
	bool openModuleManager = false; // モジュール管理画面を開くボタン押下
};
namespace Settings {
	class ModuleSettingsWindow {
	public:
		ModuleSettingsWindow(ModuleSettings settings);
		~ModuleSettingsWindow();
		ModuleSettingsWindowResult Draw(bool& isShow);
		ModuleSettings& GetTempSettings() { return tempSettings; }
	private:
		ModuleSettings tempSettings{};
	};
}
