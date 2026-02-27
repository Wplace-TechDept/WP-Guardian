#pragma once
#include "DiffPattern.h"
#include <string>

namespace Settings{
	struct WindowStates {
		bool showOriginal = true;
		bool showRealtime = true;
		bool showDiff = true;
		bool showSettings = true;
		bool showInfo = true;
		bool showUISettings = true;
	};

	struct AppSettings {
		int tile_x, tile_y;
		int pixel_x, pixel_y;
		float updateSpeed;
		DiffPattern diffPattern;
		std::string imgPath;
	};
	struct UpdateCache {
		bool foundUpdate;
		std::string latestVersion;
		std::string updateURL;
		bool showUpdateMessage;
	};
	struct UISettings{
		bool isLinkZoom;
		float UIScale;
		float WindowRounding;
		float FrameRounding;
	};
}