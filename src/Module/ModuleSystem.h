#pragma once
#include "ModuleSettings/Core/ModuleSettings.h"
#include "ModuleSettings/UI/ModuleSettingsWindow.h"
#include "AppState.h"
#include "Server/Server.h"
#include "ImWS/ImWS.h"
#include "WSVariable/WSVariable.h"
#include "ModuleManager/ModuleManager.h"
#include "Base64Codec/Base64Codec.h"
#include "Settings/Core/Settings.h"
#include "Settings/Core/SettingsJSON.h"
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
class ModuleSystem {
public:
	ModuleSystem(const ModuleSettings::ModuleSettings& settings, AppState& state);
	~ModuleSystem();
    void start();   // サーバー起動 + モジュール自動起動
    void stop();    // サーバー停止

    // メインループ内
    void update();          // imws.update() + wsVar.update()
    void drawUI();          // ModuleSettingsWindow + mm.ShowWindow
    void drawMenuItems();   // メニューバーの該当項目

    // 再起動フラグ
    bool needsRestart() const;
    ModuleSettings::ModuleSettings getUpdatedSettings() const;

private:
    AppState* state = nullptr;
    ModuleSettings::ModuleSettings settings;
    ModuleSettings::ModuleWindowStates windowStates;

    Server server;
    ImWS imws;
    WSVariable wsVar;
    ModuleManager mm;
    ModuleSettings::ModuleSettingsWindow moduleSettingsWindow;

    bool restartRequested = false;

    void bindWebSocket();
    void registerVariables();  // wsVar.register_* を全部ここに
    bool can_bind(const std::string& ip, unsigned short port);
    std::string MatToBase64(const cv::Mat& mat);

};