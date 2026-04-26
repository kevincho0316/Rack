#pragma once
#include <filesystem>
#include <cstdlib>

namespace RackPaths {
    inline const std::filesystem::path dotRack    = ".rack";
    inline const std::filesystem::path objects    = ".rack/objects";
    inline const std::filesystem::path initFile   = ".rack/init";
    inline const std::filesystem::path configFile = ".rack/config";  // stores: project

    inline std::filesystem::path globalConfigFile() {
        const char* home = std::getenv("HOME");
        return home ? std::filesystem::path(home) / ".rack" / "config"
                    : std::filesystem::path(".rack") / "config";
    }
}
