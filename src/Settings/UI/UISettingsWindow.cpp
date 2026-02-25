#include "UISettingsWindow.h"
#include <algorithm>

namespace Settings {
	UISettingsWindow::UISettingsWindow()
	{
	}
	UISettingsWindow::~UISettingsWindow()
	{
	}
	void UISettingsWindow::Draw(bool& isShow, UISettings& settings)
	{
		if (firstFrame) {
			ImGuiStyle& style = ImGui::GetStyle();
			ImGuiIO& io = ImGui::GetIO();
			style.WindowRounding = settings.WindowRounding;
			style.PopupRounding = settings.WindowRounding;
			style.FrameRounding = settings.FrameRounding;
			style.GrabRounding = settings.FrameRounding;
			io.FontGlobalScale = settings.UIScale;
			firstFrame = false;
		}

		if (!isShow) return;
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiIO& io = ImGui::GetIO();
		float UIScalePercent = settings.UIScale * 100.0f;

		ImGui::Begin("UI設定", &isShow);

		ImGui::Text("リンク拡大");
		ImGui::Checkbox("##リンク拡大", &settings.isLinkZoom);
		ImGui::Text("画面の丸み");
		// Windowの角丸
		if (ImGui::SliderFloat("ウィンドウ", &settings.WindowRounding, 0.0f, 20.0f, "%.1f")) {
			style.WindowRounding = settings.WindowRounding * settings.UIScale;
			style.PopupRounding = settings.WindowRounding * settings.UIScale;
		}

		// Frame（ボタンや入力欄）の角丸
		if (ImGui::SliderFloat("ウィジェット", &settings.FrameRounding, 0.0f, 10.0f, "%.1f")) {
			style.FrameRounding = settings.FrameRounding * settings.UIScale;
			style.GrabRounding = settings.FrameRounding * settings.UIScale;
		}


		ImGui::Text("UIスケール");
		if (ImGui::SliderFloat("##UIスケール", &UIScalePercent, 50.0f, 200.0f, "%.0f%%")) {
			UIScalePercent = std::min(UIScalePercent, 200.0f);
			UIScalePercent = std::max(UIScalePercent, 50.0f);
			io.FontGlobalScale = UIScalePercent / 100.0f;
			settings.UIScale = io.FontGlobalScale;
			style.WindowRounding = settings.WindowRounding * settings.UIScale;
			style.PopupRounding = settings.WindowRounding * settings.UIScale;
			style.FrameRounding = settings.FrameRounding * settings.UIScale;
			style.GrabRounding = settings.FrameRounding * settings.UIScale;
		}

		if (ImGui::Button("デフォルトに戻す")) {
			settings.isLinkZoom = true;
			settings.UIScale = 1.0f;
			settings.WindowRounding = 8.0f;
			settings.FrameRounding = 3.0f;
			io.FontGlobalScale = settings.UIScale;
			style.WindowRounding = settings.WindowRounding * settings.UIScale;
			style.PopupRounding = settings.WindowRounding * settings.UIScale;
			style.FrameRounding = settings.FrameRounding * settings.UIScale;
			style.GrabRounding = settings.FrameRounding * settings.UIScale;
		}

		ImGui::End();
	}
}