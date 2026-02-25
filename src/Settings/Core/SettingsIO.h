#pragma once
#include "Settings.h"

namespace Settings {
	namespace IO {
		bool Save(const WindowStates&, const AppSettings&, const ModuleSettings&, const UpdateCache&, const UISettings&);
		bool Load(WindowStates&, AppSettings&, ModuleSettings&, UpdateCache&, UISettings&);
	}
}
