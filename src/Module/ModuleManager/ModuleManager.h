#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ModuleManager {
public:
	explicit ModuleManager(const std::string& moduleListPath);
	void ShowWindow();
	void LaunchModules();
	void SetShow(bool show);
	bool IsShown() const;
private:
	bool Load();
	bool Save();
private:
	bool showWindow = false;
	bool loaded = false;

	std::string moduleListPath;
	json moduleList;
};