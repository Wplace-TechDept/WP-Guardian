#pragma once
#include "Settings.h"
#include "imgui.h"

namespace Settings {
	class UISettingsWindow {
	public:
		UISettingsWindow();
		~UISettingsWindow();
		void Draw(bool& isShow, UISettings& settings);
	private:
		bool firstFrame = true;
	};
}