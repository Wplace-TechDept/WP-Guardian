#include "ModuleSettingsJSON.h"

namespace ModuleSettings
{
	namespace JSON
	{
		bool ToJSON(json& j, const Settings& moduleSettings) {
			try {
				j["ModuleSettings"] =
				{
					{"autoStartModules", moduleSettings.autoStartModules},
					{"useServer", moduleSettings.useServer},
					{"serverIP", moduleSettings.serverIP},
					{"serverPort", moduleSettings.serverPort}
				};
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool ToJSON(json& j, const ModuleWindowStates& windowStates) {
			try {
				j["ModuleWindowStates"] = {
					{"showWindowManager", windowStates.showWindowManager},
					{"showModuleSettings", windowStates.showModuleSettings}
				};
			}
			catch (...) { return false; }
			return true;
		}
		bool FromJSON(const json& j, Settings& moduleSettings)
		{
			try {
				const auto& m = j.at("ModuleSettings");
				assign_if_exists(m, "autoStartModules", moduleSettings.autoStartModules);
				assign_if_exists(m, "useServer", moduleSettings.useServer);
				assign_if_exists(m, "serverIP", moduleSettings.serverIP);
				assign_if_exists(m, "serverPort", moduleSettings.serverPort);
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool FromJSON(const json& j, ModuleWindowStates& windowStates) {
			try {
				const auto& w = j.at("ModuleWindowStates");
				assign_if_exists(w, "showWindowManager", windowStates.showWindowManager);
				assign_if_exists(w, "showModuleSettings", windowStates.showModuleSettings);
			}
			catch (...) { return false; }
			return true;
		}
	}
}