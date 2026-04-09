#include "ModuleSettingsIO.h"
#include "ModuleSettingsJSON.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Platform/PathUtil.h"

using json = nlohmann::json;

namespace ModuleSettings
{
	namespace IO {
		bool Save(const Settings& moduleSettings, const ModuleWindowStates& moduleWindowStates) {
			try {
				json j;
				if (!JSON::ToJSON(j, moduleSettings)) return false;
				if (!JSON::ToJSON(j, moduleWindowStates)) return false;
				std::ofstream ofs(PathUtil::GetExeDir() + "module_settings.json");
				if (!ofs.is_open()) return false;
				ofs << j.dump(4);
				return true;
			}
			catch (...) {
				return false;
			}
		}
		bool Load(Settings& moduleSettings, ModuleWindowStates& moduleWindowStates) {
			try {
				std::ifstream ifs(PathUtil::GetExeDir() + "module_settings.json");
				if (!ifs.is_open()) return false;
				json j;
				ifs >> j;
				if (!JSON::FromJSON(j, moduleSettings)) return false;
				JSON::FromJSON(j, moduleWindowStates);
				return true;
			}
			catch (...) {
				return false;
			}
		}
	}
}