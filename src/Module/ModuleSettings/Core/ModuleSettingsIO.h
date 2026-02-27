#pragma once
#include "ModuleSettings.h"

namespace ModuleSettings
{
	namespace IO {
		bool Save(const ModuleSettings& moduleSettings);
		bool Load(ModuleSettings& moduleSettings);
	}
}