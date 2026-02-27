#pragma once
#include "ModuleSettings.h"

namespace ModuleSettings
{
	namespace IO {
		bool Save(const Settings& moduleSettings);
		bool Load(Settings& moduleSettings);
	}
}