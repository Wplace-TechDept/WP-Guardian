#pragma once
#include "ModuleSettings.h"
#include "nlohmann/json.hpp"
#include "JsonUtil.h"

using json = nlohmann::json;

namespace ModuleSettings
{
	namespace JSON
	{
		bool ToJSON(json& j, const Settings& moduleSettings);
		bool FromJSON(const json& j, Settings& moduleSettings);
	}
}
