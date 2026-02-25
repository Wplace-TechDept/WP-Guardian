#pragma once
#include "Settings.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Settings {
	class AppSettingsWindow {
	public:
		AppSettingsWindow(AppSettings settings,GLFWwindow* window, ImFont* bigFont);
		~AppSettingsWindow();
		bool Draw(bool& isShow);
		AppSettings& GetTempSettings() { return tempSettings; }
	private:
		AppSettings tempSettings{};
		GLFWwindow* window = nullptr;
		ImFont* bigFont = nullptr;
	};
}
