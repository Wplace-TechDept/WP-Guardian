#include "AppSettingsWindow.h"
#include <windows.h>
namespace Settings
{
    AppSettingsWindow::AppSettingsWindow(AppSettings settings,GLFWwindow* window, ImFont* bigFont)
        :tempSettings(settings), window(window), bigFont(bigFont) {
    }

    AppSettingsWindow::~AppSettingsWindow() {
    }

    bool AppSettingsWindow::Draw(bool& isShow) {
        bool Update = false;
        if (!isShow) return false;

        ImGui::Begin("設定", &isShow);

        float windowWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = windowWidth * 0.9f;
        float xPos = (windowWidth - itemWidth) * 0.5f;

        // imgPath用バッファ（安全に固定長）
        char imgPathBuf[MAX_PATH] = {};
        strncpy(imgPathBuf, tempSettings.imgPath.c_str(), MAX_PATH - 1);

        // --- 座標系 ---
        ImGui::SetCursorPosX(xPos);
        ImGui::PushFont(bigFont);
        ImGui::Text("座標");
        ImGui::PopFont();

        ImGui::PushItemWidth(itemWidth);
        ImGui::SetCursorPosX(xPos);
        ImGui::InputInt("Tl X", &tempSettings.tile_x);
        ImGui::SetCursorPosX(xPos);
        ImGui::InputInt("Tl Y", &tempSettings.tile_y);
        ImGui::SetCursorPosX(xPos);
        ImGui::InputInt("Px X", &tempSettings.pixel_x);
        ImGui::SetCursorPosX(xPos);
        ImGui::InputInt("Px Y", &tempSettings.pixel_y);
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // --- 更新間隔 ---
        ImGui::SetCursorPosX(xPos);
        ImGui::PushFont(bigFont);
        ImGui::Text("更新間隔");
        ImGui::PopFont();

        ImGui::PushItemWidth(itemWidth);
        ImGui::SetCursorPosX(xPos);
        ImGui::InputFloat("秒", &tempSettings.updateSpeed, 0.1f, 1.0f, "%.1f");
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // --- 差分形式 ---
        ImGui::SetCursorPosX(xPos);
        ImGui::PushFont(bigFont);
        ImGui::Text("差分形式");
        ImGui::PopFont();

        ImGui::PushItemWidth(itemWidth);
        ImGui::SetCursorPosX(xPos);
        ImGui::Combo("##差分形式", (int*)&tempSettings.diffPattern.type,
            "差分計算表示\0差分部分を強調色\0差分以外をテンプレ表示\0");
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // 差分形式に応じてマスク色
        if (tempSettings.diffPattern.type == DiffPatternType::ColorMask ||
            tempSettings.diffPattern.type == DiffPatternType::OverlayTemplate) {

            ImVec4 tmpMaskColor{
                tempSettings.diffPattern.maskColor.r,
                tempSettings.diffPattern.maskColor.g,
                tempSettings.diffPattern.maskColor.b,
                tempSettings.diffPattern.maskColor.a
            };

            ImGui::SetCursorPosX(xPos);
            ImGui::PushFont(bigFont);
            ImGui::Text("使用する色");
            ImGui::PopFont();

            ImGui::PushItemWidth(itemWidth);
            ImGui::SetCursorPosX(xPos);
            if (ImGui::ColorEdit4("##マスク色", (float*)&tmpMaskColor,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {

                tempSettings.diffPattern.maskColor.r = tmpMaskColor.x;
                tempSettings.diffPattern.maskColor.g = tmpMaskColor.y;
                tempSettings.diffPattern.maskColor.b = tmpMaskColor.z;
                tempSettings.diffPattern.maskColor.a = tmpMaskColor.w;
            }
            ImGui::PopItemWidth();
            ImGui::Spacing();
        }

        // --- 画像パス ---
        ImGui::SetCursorPosX(xPos);
        ImGui::PushFont(bigFont);
        ImGui::Text("使用画像パス");
        ImGui::PopFont();

        ImGui::PushItemWidth(itemWidth - 80);
        ImGui::SetCursorPosX(xPos);
        if (ImGui::InputText("##画像保存先", imgPathBuf, MAX_PATH)) {
            tempSettings.imgPath = std::string(imgPathBuf); // 常に正しい長さでコピー
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::SetCursorPosX(xPos + itemWidth - 80);
        if (ImGui::Button("参照", ImVec2(80, 0))) {
            OPENFILENAME ofn{};
            ofn.lStructSize = sizeof(ofn);
            HWND hwnd = glfwGetWin32Window(window);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = imgPathBuf;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter =
                "PNG Files\0*.png\0"
                "JPEG Files\0*.jpg;*.jpeg\0"
                "All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                tempSettings.imgPath = std::string(imgPathBuf); // 正しい長さでコピー
            }
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX(xPos);
        if (ImGui::Button("更新", ImVec2(itemWidth, 0))) {
            Update = true;
        }

        ImGui::End();
        return Update;
    }

}