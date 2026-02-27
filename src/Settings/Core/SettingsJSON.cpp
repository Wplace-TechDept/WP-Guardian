#include "SettingsJSON.h"

namespace Settings
{
	namespace JSON
	{
		bool ToJSON(json& j, const WindowStates& windowStates)
		{
			try {
				j["WindowStates"] = {
					{"showOriginal", windowStates.showOriginal},
					{"showRealtime", windowStates.showRealtime},
					{"showDiff", windowStates.showDiff},
					{"showSettings", windowStates.showSettings},
					{"showInfo", windowStates.showInfo},
					{"showUISettings", windowStates.showUISettings }
				};
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool ToJSON(json& j, const AppSettings& appSettings)
		{
			try {
				j["AppSettings"] =
				{
					{"tile_x", appSettings.tile_x},
					{"tile_y", appSettings.tile_y},
					{"pixel_x", appSettings.pixel_x},
					{"pixel_y", appSettings.pixel_y},
					{"updateSpeed", appSettings.updateSpeed},
					{"imgPath", appSettings.imgPath}
				};
				j["AppSettings"]["diffPattern"]["type"] = static_cast<int>(appSettings.diffPattern.type);
				j["AppSettings"]["diffPattern"]["maskColor"] = {
					{"r", appSettings.diffPattern.maskColor.r},
					{"g", appSettings.diffPattern.maskColor.g},
					{"b", appSettings.diffPattern.maskColor.b},
					{"a", appSettings.diffPattern.maskColor.a}
				};
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool ToJSON(json& j, const UpdateCache& updateCache) {
			try {
				j["UpdateCache"] =
				{
					{"foundUpdate", updateCache.foundUpdate},
					{"latestVersion", updateCache.latestVersion},
					{"updateURL", updateCache.updateURL},
					{"showUpdateMessage", updateCache.showUpdateMessage }
				};
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool ToJSON(json& j, const UISettings& uiSettings) {
			try {
				j["UISettings"] =
				{
					{"isLinkZoom", uiSettings.isLinkZoom},
					{"UIScale", uiSettings.UIScale},
					{"WindowRounding", uiSettings.WindowRounding},
					{"FrameRounding", uiSettings.FrameRounding}
				};
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool FromJSON(const json& j, WindowStates& windowStates)
		{
			try {
				const auto& windowstate = j.at("WindowStates");

				assign_if_exists(windowstate, "showOriginal", windowStates.showOriginal);
				assign_if_exists(windowstate, "showRealtime", windowStates.showRealtime);
				assign_if_exists(windowstate, "showDiff", windowStates.showDiff);
				assign_if_exists(windowstate, "showSettings", windowStates.showSettings);
				assign_if_exists(windowstate, "showInfo", windowStates.showInfo);
				assign_if_exists(windowstate, "showUISettings", windowStates.showUISettings);
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool FromJSON(const json& j, AppSettings& appSettings)
		{
			try {
				const auto& a = j.at("AppSettings");

				assign_if_exists(a, "tile_x", appSettings.tile_x);
				assign_if_exists(a, "tile_y", appSettings.tile_y);
				assign_if_exists(a, "pixel_x", appSettings.pixel_x);
				assign_if_exists(a, "pixel_y", appSettings.pixel_y);
				assign_if_exists(a, "updateSpeed", appSettings.updateSpeed);
				assign_if_exists(a, "imgPath", appSettings.imgPath);

				if (a.contains("diffPattern"))
				{
					const auto& d = a["diffPattern"];

					if (d.contains("type")) {
						int t = d["type"].get<int>();
						appSettings.diffPattern.type =
							static_cast<DiffPatternType>(t);
					}


					if (d.contains("maskColor"))
					{
						const auto& m = d["maskColor"];
						assign_if_exists(m, "r", appSettings.diffPattern.maskColor.r);
						assign_if_exists(m, "g", appSettings.diffPattern.maskColor.g);
						assign_if_exists(m, "b", appSettings.diffPattern.maskColor.b);
						assign_if_exists(m, "a", appSettings.diffPattern.maskColor.a);
					}
				}
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool FromJSON(const json& j, UpdateCache& updateCache)
		{
			try {
				const auto& u = j.at("UpdateCache");
				assign_if_exists(u, "foundUpdate", updateCache.foundUpdate);
				assign_if_exists(u, "latestVersion", updateCache.latestVersion);
				assign_if_exists(u, "updateURL", updateCache.updateURL);
				assign_if_exists(u, "showUpdateMessage", updateCache.showUpdateMessage);
			}
			catch (...) {
				return false;
			}
			return true;
		}
		bool FromJSON(const json& j, UISettings& uiSettings)
		{
			try {
				const auto& u = j.at("UISettings");
				assign_if_exists(u, "isLinkZoom", uiSettings.isLinkZoom);
				assign_if_exists(u, "UIScale", uiSettings.UIScale);
				assign_if_exists(u, "WindowRounding", uiSettings.WindowRounding);
				assign_if_exists(u, "FrameRounding", uiSettings.FrameRounding);
			}
			catch (...) {
				return false;
			}
			return true;
		}
	}
}