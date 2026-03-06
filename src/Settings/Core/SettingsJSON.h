#pragma once 
#include "Settings.h"
#include "nlohmann/json.hpp"
#include "JsonUtil.h"


using json = nlohmann::json;

namespace Settings {
	namespace JSON {
		bool ToJSON(json& j, const WindowStates& windowStates);
		bool ToJSON(json& j, const AppSettings& appSettings);
		bool ToJSON(json& j, const UpdateCache& updateCache);
		bool ToJSON(json& j, const UISettings& uiSettings);
		bool FromJSON(const json& j, WindowStates& windowStates);
		bool FromJSON(const json& j, AppSettings& appSettings);
		bool FromJSON(const json& j, UpdateCache& updateCache);
		bool FromJSON(const json& j, UISettings& uiSettings);
	}
}
