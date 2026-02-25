#include "ModuleSettingsWindow.h"

namespace Settings {
    ModuleSettingsWindow::ModuleSettingsWindow(ModuleSettings settings)
        : tempSettings(settings) {
    }

    ModuleSettingsWindow::~ModuleSettingsWindow() {
    }

    ModuleSettingsWindowResult ModuleSettingsWindow::Draw(bool& isShow) {
        ModuleSettingsWindowResult result{};
        result.updated = false;
        result.openModuleManager = false;
        if (!isShow) return result;

        ImGui::Begin("モジュール設定", &isShow);
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float itemWidth = ImGui::GetContentRegionAvail().x * 0.9f;
        float xPos = (windowWidth - itemWidth) * 0.5f;

        char ipBuf[128] = {};
        // 現在の値をバッファにコピー (安全のため strncpy 等を使用)
        strncpy(ipBuf, tempSettings.serverIP.c_str(), sizeof(ipBuf) - 1);

        ImGui::PushItemWidth(itemWidth);
        ImGui::SetCursorPosX(xPos);
        ImGui::Text("モジュールの動作(主にサーバーに関する)設定です。");
        ImGui::SetCursorPosX(xPos);
        ImGui::Text("設定の変更を保存する際はアプリを再起動します");
        ImGui::Spacing();

        ImGui::SetCursorPosX(xPos);
        ImGui::Text("モジュールを使用するか");
        ImGui::SetCursorPosX(xPos);
        ImGui::Checkbox("##useServer", &tempSettings.useServer);

        if (tempSettings.useServer)
        {
            ImGui::SetCursorPosX(xPos);
            ImGui::Text("モジュールを自動で起動するか");
            ImGui::SetCursorPosX(xPos);
            ImGui::Checkbox("##autoStartModules", &tempSettings.autoStartModules);

            ImGui::SetCursorPosX(xPos);
            if (ImGui::Button("モジュールの管理"))
            {
                result.openModuleManager = true;
            }

            ImGui::SetCursorPosX(xPos);
            ImGui::Text("WebSocketサーバー ポート番号:");
            ImGui::SetCursorPosX(xPos);
            if (ImGui::InputInt("##port", &tempSettings.serverPort))
            {
                if (tempSettings.serverPort < 0)
                    tempSettings.serverPort = 0;
                else if (tempSettings.serverPort > 65535)
                    tempSettings.serverPort = 65535;
            }

            ImGui::SetCursorPosX(xPos);
            ImGui::Text("サーバーIPアドレス:");
            ImGui::SetCursorPosX(xPos);
            if (ImGui::InputText("##ip", ipBuf, sizeof(ipBuf)))
            {
                tempSettings.serverIP = std::string(ipBuf);
            }
        }
        else
        {
            tempSettings.autoStartModules = false;
            ImGui::SetCursorPosX(xPos);
            ImGui::Text("モジュールを無効にすると、サーバー自体が起動しなくなります。");
        }

        ImGui::PopItemWidth();
        ImGui::SetCursorPosX(xPos);
        if (ImGui::Button("更新", ImVec2(itemWidth, 0))) {
            result.updated = true;
        }
        ImGui::End();
        return result;
    }
}