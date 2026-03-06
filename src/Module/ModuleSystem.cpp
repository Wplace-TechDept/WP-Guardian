#include "ModuleSystem.h"
#include "imgui.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include "ModuleSettings/Core/ModuleSettingsJSON.h"
#include "ModuleSettings/Core/ModuleSettingsIO.h"
#include "SettingsJSON.h"

using json = nlohmann::json;
ModuleSystem::ModuleSystem(AppState& state)
	: state(state), mm("assets/moduleList.json"), moduleSettingsWindow(ModuleSettings::Settings{})
{
	bindWebSocket();
	registerVariables();
	// settingsはstart()→loadSettings()で上書きされる
}
ModuleSystem::~ModuleSystem()
{
	stop();
}
void ModuleSystem::bindWebSocket()
{
	server.add_open_handler([this](websocketpp::connection_hdl hdl) {
		imws.on_open(hdl);
		wsVar.on_open(hdl);
	});
	server.add_close_handler([this](websocketpp::connection_hdl hdl) {
		imws.on_close(hdl);
		wsVar.on_close(hdl);
	});
	server.add_message_handler([this](websocketpp::connection_hdl hdl, WSServer::message_ptr msg) {
		imws.on_message(hdl, msg);
		wsVar.on_message(hdl, msg);
	});
	imws.send = [this](websocketpp::connection_hdl hdl, const std::string& msg) {
		server.send(hdl, msg);
	};
	wsVar.send = [this](websocketpp::connection_hdl hdl, const std::string& msg) {
		server.send(hdl, msg);
	};
}
void ModuleSystem::registerVariables()
{
	wsVar.register_setter("update", [&](const json& value) {
		state.update = value.get<bool>();
	});
	wsVar.register_getter("settings.temp", [&]() -> json {
		json j;
		Settings::JSON::ToJSON(j, state.getTempSettings());
		return j["AppSettings"];
	});
	wsVar.register_setter("settings.temp", [&](const json& value) {
		Settings::AppSettings temp;
		json wrapper;
		wrapper["AppSettings"] = value;
		Settings::JSON::FromJSON(wrapper, temp);
		state.setTempSettings(temp);
	});
	wsVar.register_getter("settings", [&]() -> json {
		json j;
		Settings::JSON::ToJSON(j, *state.appSettings);
		return j["AppSettings"];
	});

	wsVar.register_getter("info", [&]() -> json {
		json j;
		j["totalPixels"] = state.totalPixels;
		j["diffPixels"] = state.diffPixels;
		j["diffPercent"] = state.diffPercent;
		return j;
	});
	wsVar.register_getter("image.size", [&]() -> json {
		return{
			{"width", state.width},
			{"height", state.height}
		};
	});
	wsVar.register_getter("image.original", [&]() -> json {
		if (!state.originalBase64.hasData) {
			state.originalBase64.data = MatToBase64(state.originalImg);
			state.originalBase64.hasData = true;
		}
		return{
			{"width", state.originalImg.cols},
			{"height", state.originalImg.rows},
			{"data", state.originalBase64.data}
		};
	});
	wsVar.register_getter("image.realtime", [&]() -> json {
		if (!state.realtimeBase64.hasData) {
			state.realtimeBase64.data = MatToBase64(state.realtimeImg);
			state.realtimeBase64.hasData = true;
		}
		return{
			{"width", state.realtimeImg.cols},
			{"height", state.realtimeImg.rows},
			{"data", state.realtimeBase64.data}
		};
	});
	wsVar.register_getter("image.diff", [&]() -> json {
		if (!state.diffBase64.hasData) {
			state.diffBase64.data = MatToBase64(state.diffImg);
			state.diffBase64.hasData = true;
		}
		return{
			{"width", state.diffImg.cols},
			{"height", state.diffImg.rows},
			{"data", state.diffBase64.data}
		};
	});
}

std::string ModuleSystem::MatToBase64(const cv::Mat& mat)
{
	// 元は BGRA
	CV_Assert(mat.type() == CV_8UC4);
	CV_Assert(mat.isContinuous());

	cv::Mat rgbaMat;
	cv::cvtColor(mat, rgbaMat, cv::COLOR_BGRA2RGBA); // B G R A → R G B A

	const uint8_t* data = rgbaMat.data;
	size_t dataSize = rgbaMat.total() * rgbaMat.elemSize();

	static Base64Codec codec;
	return codec.encode(data, dataSize);
}

bool ModuleSystem::can_bind(const std::string& ip, unsigned short port)
{
	try {
		asio::io_context ctx;
		asio::ip::tcp::acceptor acceptor(ctx);
		asio::ip::tcp::endpoint ep(asio::ip::make_address(ip), port);
		acceptor.open(ep.protocol());
		acceptor.bind(ep);  // ここでポート使用中なら例外
		acceptor.close();
		return true; // バインド可能
	}
	catch (const std::exception&) {
		return false; // 既に使用中などでバインド不可
	}
}

void ModuleSystem::start()
{
	loadSettings();
	if (settings.useServer) {
		if (can_bind(settings.serverIP, settings.serverPort)) {
			server.start(settings.serverIP, settings.serverPort);
		}
		else {
			std::cerr << "ポートがすでに使用されています: " << settings.serverPort << "。サーバーを起動しません。" << std::endl;
			settings.useServer = false;
			settings.autoStartModules = false;
		}
	}
	if (settings.autoStartModules) {
		mm.LaunchModules();
	}
}
void ModuleSystem::stop()
{
	saveSettings();
	server.stop();
}
void ModuleSystem::update()
{
	imws.update();
	wsVar.update();
}
void ModuleSystem::drawUI()
{
	imws.drawWindowManager(windowStates.showWindowManager);
	auto result = moduleSettingsWindow.Draw(windowStates.showModuleSettings);
	if (result.openModuleManager) {
		mm.SetShow(true);
	}
	if (result.updated) {
		restartRequested = true;
	}
	mm.ShowWindow();
}
void ModuleSystem::drawWindowMenuItems()
{
	ImGui::MenuItem("ウィンドウ管理", nullptr, &windowStates.showWindowManager);
	ImGui::MenuItem("モジュール設定", nullptr, &windowStates.showModuleSettings);
}
void ModuleSystem::drawModuleManagerMenuItems()
{
	if (ImGui::MenuItem("モジュール管理")) {
		mm.SetShow(true);
	}
}
bool ModuleSystem::needsRestart() const {
	return restartRequested;
}
ModuleSettings::Settings ModuleSystem::getUpdatedSettings() const {
	return moduleSettingsWindow.GetTempSettings();
}
void ModuleSystem::loadSettings()
{
	ModuleSettings::Settings loadedSettings;
	ModuleSettings::ModuleWindowStates loadedWindowStates;
	if (ModuleSettings::IO::Load(loadedSettings, loadedWindowStates)) {
		settings = loadedSettings;
		windowStates = loadedWindowStates;
		moduleSettingsWindow = ModuleSettings::ModuleSettingsWindow(settings);
	}
	else {
		std::cerr << "モジュール設定の読み込みに失敗しました。デフォルト設定を使用します。" << std::endl;
	}
}
void ModuleSystem::saveSettings()
{
	ModuleSettings::Settings currentSettings = moduleSettingsWindow.GetTempSettings();
	ModuleSettings::ModuleWindowStates currentWindowStates = windowStates;
	if (!ModuleSettings::IO::Save(currentSettings, currentWindowStates)) {
		std::cerr << "モジュール設定の保存に失敗しました。" << std::endl;
	}
}