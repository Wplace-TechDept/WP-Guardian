#include "SettingsIO.h"
#include "SettingsJSON.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Platform/PathUtil.h"

using json = nlohmann::json;

namespace Settings {
    namespace IO {

        bool Save(const WindowStates& windowStates, const AppSettings& appSettings, const UpdateCache& updateCache, const UISettings& uiSettings) {
            try {
                json j;

                // SettingsJSON �� ToJSON ���g��
                if (!JSON::ToJSON(j, windowStates)) return false;
                if (!JSON::ToJSON(j, appSettings)) return false;
                if (!JSON::ToJSON(j, updateCache)) return false;
				if (!JSON::ToJSON(j, uiSettings)) return false;


                std::ofstream ofs(PlatformUtil::Path::GetExeDir() + "settings.json");
                if (!ofs.is_open()) return false;
                ofs << j.dump(4);

                return true;
            }
            catch (...) {
                return false;
            }
        }

        bool Load(WindowStates& windowStates, AppSettings& appSettings, UpdateCache& updateCache, UISettings& uiSettings) {
            try {
                std::ifstream ifs(PlatformUtil::Path::GetExeDir() + "settings.json");
                if (!ifs.is_open()) return false;

                json j;
                ifs >> j;

                // SettingsJSON �� FromJSON ���g��
                if (!JSON::FromJSON(j, windowStates)) return false;
                if (!JSON::FromJSON(j, appSettings)) return false;
                if (!JSON::FromJSON(j, updateCache)) return false;
				if (!JSON::FromJSON(j, uiSettings)) return false;

                return true;
            }
            catch (...) {
                return false;
            }
        }

    } // namespace IO
} // namespace Settings
