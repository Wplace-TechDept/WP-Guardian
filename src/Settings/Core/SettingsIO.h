#pragma once
#include "Settings.h"

namespace Settings {
	namespace IO {
		bool Save(const WindowStates&, const AppSettings&, const UpdateCache&, const UISettings&);
		bool Load(WindowStates&, AppSettings&, UpdateCache&, UISettings&);
	}
}
