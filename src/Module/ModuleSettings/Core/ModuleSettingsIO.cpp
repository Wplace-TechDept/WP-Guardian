#include "ModuleSettingsIO.h"
#include "ModuleSettingsJSON.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "PathUtil.h"

using json = nlohmann::json;

namespace ModuleSettings
{
	namespace IO {
		bool Save(const Settings& moduleSettings) {
			try {
				json j;
				// ModuleSettingsJSON ‚Ì ToJSON ‚ðŽg‚¤
				if (!JSON::ToJSON(j, moduleSettings)) return false;
				std::ofstream ofs(GetExeDir() + "module_settings.json");
				if (!ofs.is_open()) return false;
				ofs << j.dump(4);
				return true;
			}
			catch (...) {
				return false;
			}
		}
		bool Load(Settings& moduleSettings) {
			try {
				std::ifstream ifs(GetExeDir() + "module_settings.json");
				if (!ifs.is_open()) return false;
				json j;
				ifs >> j;
				// ModuleSettingsJSON ‚Ì FromJSON ‚ðŽg‚¤
				if (!JSON::FromJSON(j, moduleSettings)) return false;
				return true;
			}
			catch (...) {
				return false;
			}
		}
	}
}