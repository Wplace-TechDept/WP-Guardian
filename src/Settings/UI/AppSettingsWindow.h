#pragma once
#include "Settings.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#ifndef __EMSCRIPTEN__
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace Settings {
	class AppSettingsWindow {
	public:
		AppSettingsWindow(AppSettings settings,GLFWwindow* window, ImFont* bigFont);
		~AppSettingsWindow();
		bool Draw(bool& isShow);
		AppSettings& GetTempSettings() { return tempSettings; }
		void SetTempSettings(const AppSettings& s) { tempSettings = s; }
	private:
		AppSettings tempSettings{};
		GLFWwindow* window = nullptr;
		ImFont* bigFont = nullptr;
	};
}
