#include "ModuleManager.h"
#include "imgui.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "PathUtil.h"

ModuleManager::ModuleManager(const std::string& path)
{
    if (std::filesystem::path(path).is_absolute()) {
        moduleListPath = path;
    }
    else {
        moduleListPath = PathUtil::GetExeDir() + path;
    }
}


// --- JSON読み込み ---
bool ModuleManager::Load()
{
    std::ifstream file(moduleListPath);
    if (!file.is_open()) {
        std::cerr << "moduleList.json が開けません: " << moduleListPath << std::endl;
        return false;
    }
    file >> moduleList;
    file.close();
    return true;
}

// --- JSON保存 ---
bool ModuleManager::Save()
{
    std::ofstream file(moduleListPath);
    if (!file.is_open()) {
        std::cerr << "moduleList.json の保存に失敗: " << moduleListPath << std::endl;
        return false;
    }
    file << moduleList.dump(4);
    file.close();
    return true;
}

void ModuleManager::ShowWindow()
{
    if (!showWindow) return;

    if (!ImGui::Begin("モジュール管理", &showWindow)) {
        ImGui::End();
        return;
    }

    if (!loaded) {
        if (!Load())
            moduleList["modules"] = nlohmann::json::array();
        loaded = true;
    }

    ImGui::BeginChild("ModuleListArea", ImVec2(0, 400), true);

    if (moduleList.contains("modules")) {
        int deleteIndex = -1;

        for (size_t i = 0; i < moduleList["modules"].size(); ++i) {
            auto& module = moduleList["modules"][i];

            bool enabled = module.value("enabled", true);
            if (ImGui::Checkbox(("有効##" + std::to_string(i)).c_str(), &enabled))
                module["enabled"] = enabled;

            char nameBuf[128];
            strncpy(nameBuf, module.value("name", "").c_str(), sizeof(nameBuf));
            if (ImGui::InputText(("名前##" + std::to_string(i)).c_str(), nameBuf, sizeof(nameBuf)))
                module["name"] = nameBuf;

            char cmdBuf[256];
            strncpy(cmdBuf, module.value("command", "").c_str(), sizeof(cmdBuf));
            if (ImGui::InputText(("コマンド##" + std::to_string(i)).c_str(), cmdBuf, sizeof(cmdBuf)))
                module["command"] = cmdBuf;

            ImGui::SameLine();
            if (ImGui::Button(("削除##" + std::to_string(i)).c_str()))
                deleteIndex = (int)i;

            ImGui::Separator();
        }

        if (deleteIndex != -1)
            moduleList["modules"].erase(moduleList["modules"].begin() + deleteIndex);
    }

    ImGui::EndChild();

    if (ImGui::Button("新しいモジュールを追加")) {
        moduleList["modules"].push_back({
            {"enabled", true},
            {"name", "新しいモジュール"},
            {"command", ""}
            });
    }

    if (ImGui::Button("セーブ"))
        Save();

    ImGui::End();

    if (!showWindow) {
        moduleList.clear();
        loaded = false;
    }
}
void ModuleManager::LaunchModules()
{
    if (!Load()) return;

    for (auto& module : moduleList["modules"]) {
        if (!module.value("enabled", true)) continue;

        std::string command = module.value("command", "");
        if (command.empty()) continue;

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        if (!CreateProcessA(
            NULL, (LPSTR)command.c_str(),
            NULL, NULL, FALSE,
            CREATE_NO_WINDOW,
            NULL, NULL, &si, &pi))
        {
            DWORD err = GetLastError();
            std::cerr << "[ModuleManager] CreateProcess failed\n";
            std::cerr << "  command: " << command << "\n";
            std::cerr << "  error  : " << err << "\n";
        }
        else
        {
            std::cout << "[ModuleManager] Launched: " << command << "\n";
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

    }
}

void ModuleManager::SetShow(bool show) {
    showWindow = show;
}
bool ModuleManager::IsShown() const {
    return showWindow;
}