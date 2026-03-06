#pragma once
#include "ModuleSettings/Core/ModuleSettings.h"
#include "ModuleSettings/UI/ModuleSettingsWindow.h"
#include "AppState.h"
#include "Server/Server.h"
#include "ImWS/ImWS.h"
#include "WSVariable/WSVariable.h"
#include "ModuleManager/ModuleManager.h"
#include "Base64Codec/Base64Codec.h"
#include <string>

class ModuleSystem {
public:
    ModuleSystem(AppState& state);  // settings引数を削除
	~ModuleSystem();
    void start();   // サーバー起動 + モジュール自動起動
    void stop();    // サーバー停止

    // メインループ内
    void update();          // imws.update() + wsVar.update()
    void drawUI();          // ModuleSettingsWindow + mm.ShowWindow
    void drawWindowMenuItems();   // メニューバーの該当項目
	void drawModuleManagerMenuItems();   // モジュールごとのメニュー項目

    // 再起動フラグ
    bool needsRestart() const;
    ModuleSettings::Settings getUpdatedSettings() const;

private:
    AppState& state;
    ModuleSettings::Settings settings;
    ModuleSettings::ModuleWindowStates windowStates;

    Server server;
    ImWS imws;
    WSVariable wsVar;
    ModuleManager mm{ "assets/moduleList.json" };
    ModuleSettings::ModuleSettingsWindow moduleSettingsWindow;

    bool restartRequested = false;

	void loadSettings();
	void saveSettings();
    void bindWebSocket();
    void registerVariables();  // wsVar.register_* を全部ここに
    bool can_bind(const std::string& ip, unsigned short port);
    std::string MatToBase64(const cv::Mat& mat);

};