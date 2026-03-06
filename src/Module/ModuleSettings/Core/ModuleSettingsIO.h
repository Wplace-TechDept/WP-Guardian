#pragma once
#include "ModuleSettings.h"

namespace ModuleSettings
{
	namespace IO {
		bool Save(const Settings&, const ModuleWindowStates&);
		bool Load(Settings&, ModuleWindowStates&);
	}
}