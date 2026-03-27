#include "VersionManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#ifndef __EMSCRIPTEN__
#include <cpr/cpr.h>
#endif
using json = nlohmann::json;

VersionManager::VersionManager(const std::string& owner, const std::string& repo, const std::string& APP_VERSION)
    : owner(owner), repo(repo), APP_VERSION(APP_VERSION) {
}

VersionManager::Version VersionManager::parseVersion(const std::string& v) {
    std::string s = (v[0] == 'v') ? v.substr(1) : v;

    Version ver{};
    ver.major = ver.minor = ver.patch = 0;

    int count = sscanf(s.c_str(), "%d.%d.%d", &ver.major, &ver.minor, &ver.patch);
    return ver;
}

#ifdef __EMSCRIPTEN__
std::string VersionManager::getLatestVersion() { return ""; }
VersionManager::UpdateResult VersionManager::checkUpdate() {
    return UpdateResult{false, APP_VERSION, "", ""};
}
#else
std::string VersionManager::getLatestVersion() {
    std::string url = "https://api.github.com/repos/" + owner + "/" + repo + "/releases/latest";

    cpr::Response r = cpr::Get(
        cpr::Url{ url },
        cpr::Header{ {"User-Agent", "MyApp"} }
    );

    if (r.status_code != 200) {
        std::cerr << "Failed: " << r.status_code << "\n";
        return "";
    }

    auto json = nlohmann::json::parse(r.text);
    if (!json.contains("tag_name")) return "";

    return json["tag_name"];
}
VersionManager::UpdateResult VersionManager::checkUpdate() {
    UpdateResult result{};
    result.hasUpdate = false;
    result.localVersion = APP_VERSION;
    result.latestVersion = "";
    result.updateURL = "";

    std::string latest = getLatestVersion();
    if (latest.empty()) {
        std::cout << "Failed to get latest version" << std::endl;
        return result;
    }

    Version local = parseVersion(APP_VERSION);
    Version remote = parseVersion(latest);

    if (isNewer(remote, local)) {
        result.latestVersion = latest;
        result.updateURL = "https://github.com/"+owner+"/"+repo+"/releases/tag/" + latest;
        result.hasUpdate = true;
    }
    else {
		result.latestVersion = latest;
        result.updateURL = "https://github.com/" + owner + "/" + repo + "/releases/tag/" + latest;
		result.hasUpdate = false;
    }
	return result;

}
#endif
bool VersionManager::isNewer(const VersionManager::Version& remote, const VersionManager::Version& local) {
    if (remote.major != local.major) return remote.major > local.major;
    if (remote.minor != local.minor) return remote.minor > local.minor;
    return remote.patch > local.patch;
}
