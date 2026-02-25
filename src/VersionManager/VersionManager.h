#pragma once
#include <string>

class VersionManager {
public:
    VersionManager(const std::string& owner, const std::string& repo, const std::string& APP_VERSION);
    struct UpdateResult {
        bool hasUpdate;
        std::string localVersion;
        std::string latestVersion;
        std::string updateURL;
    };
    UpdateResult checkUpdate();
private:
    struct Version {
        int major = 0;
        int minor = 0;
        int patch = 0;
    };
    Version parseVersion(const std::string& v);
	std::string getLatestVersion();
    bool isNewer(const Version& remote, const Version& local);

private:
    std::string owner;
    std::string repo;
    std::string APP_VERSION;
};