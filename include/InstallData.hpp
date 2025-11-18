#ifndef INSTALL_DATA_HPP
#define INSTALL_DATA_HPP

#include <string>
#include <unordered_map>

enum class Platform
{
    LINUX = 0x00,
    WINDOWS = 0x01,
    MACOS = 0x02,
    NONE = 0x03
};

inline std::string fromPlatformToString(Platform platform)
{
    switch (platform)
    {
        case Platform::LINUX: return "linux";
        case Platform::MACOS: return "macos";
        case Platform::WINDOWS: return "windows";
        default: return "none";
    }
}

inline Platform fromStringToPlatform(const std::string& platform)
{
    const static std::unordered_map<std::string, Platform> platforms
    {
        {"linux", Platform::LINUX},
        {"macos", Platform::MACOS},
        {"windows", Platform::WINDOWS},
    };

    if(auto it = platforms.find(platform); it != platforms.end())
        return it->second;

    return Platform::NONE;
}

namespace install
{
    struct InstallData
    {
    public:
        std::string installPath;
        std::string version;
        Platform platform{Platform::NONE};
        
    };
} //namespace install

#endif //INSTALL_DATA_HPP