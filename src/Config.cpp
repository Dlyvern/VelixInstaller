#include "Config.hpp"
#include <cstring>

namespace
{
std::string getHomeDirectory()
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path)))
        return path;

    throw std::runtime_error("Failed to get user profile directory");
#else
    const char* homeDir = std::getenv("HOME");

    if (homeDir && std::strlen(homeDir) > 0)
        return homeDir;

    if (const auto* pwd = getpwuid(getuid()))
        return pwd->pw_dir;

    throw std::runtime_error("Cannot find home directory");
#endif
}

std::string getDefaultInstallRoot()
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path)))
        return (std::filesystem::path(path) / "Velix").string();

    return (std::filesystem::path(getHomeDirectory()) / "AppData" / "Roaming" / "Velix").string();
#else
    return (std::filesystem::path(getHomeDirectory()) / ".local" / "share" / "Velix").string();
#endif
}

std::string getDefaultProjectsRoot()
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path)))
        return (std::filesystem::path(path) / "ElixProjects").string();

    return (std::filesystem::path(getHomeDirectory()) / "Documents" / "ElixProjects").string();
#else
    return (std::filesystem::path(getHomeDirectory()) / "Documents" / "ElixProjects").string();
#endif
}
}

bool Config::load(const std::string& path)
{
    const std::string configPath = path.empty() ? getConfigPath() : path;
    m_loadedPath = configPath;
    std::cout << "[Config] Loading config from: " << configPath << std::endl;

    if(std::filesystem::exists(configPath))
    {
        std::ifstream ifs(configPath);

        if(!ifs)
            return false;

        try
        {
            ifs >> m_config;
        }
        catch (const std::exception&)
        {
            m_config = nlohmann::json::object();
        }
    }
    else
        m_config = nlohmann::json::object();

    ensureDefaults();

    return save(configPath);
}

bool Config::save(const std::string& path)
{
    const std::string targetPath = !path.empty() ? path : (!m_loadedPath.empty() ? m_loadedPath : getConfigPath());
    std::filesystem::create_directories(std::filesystem::path(targetPath).parent_path());

    std::ofstream ofs(targetPath);
    if (!ofs)
        return false;

    ofs << m_config.dump(4);
    return static_cast<bool>(ofs);
}

const nlohmann::json& Config::getConfig() const
{
    return m_config;
}

nlohmann::json& Config::mutableConfig()
{
    return m_config;
}

void Config::ensureDefaults()
{
    if (!m_config.is_object())
        m_config = nlohmann::json::object();

    if (!m_config.contains("schema_version") || !m_config["schema_version"].is_number_integer())
        m_config["schema_version"] = 2;

    if (!m_config.contains("current_version") || !m_config["current_version"].is_string())
        m_config["current_version"] = "";

    if (!m_config.contains("install_root") || !m_config["install_root"].is_string())
        m_config["install_root"] = getDefaultInstallRoot();

    if (!m_config.contains("project_root") || !m_config["project_root"].is_string())
        m_config["project_root"] = getDefaultProjectsRoot();

    if (!m_config.contains("installed_versions") || !m_config["installed_versions"].is_array())
        m_config["installed_versions"] = nlohmann::json::array();

    if (!m_config.contains("projects") || !m_config["projects"].is_array())
        m_config["projects"] = nlohmann::json::array();
}

std::string Config::getConfigPath()
{
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path)))
    {
        std::filesystem::path configPath = std::filesystem::path(path) / "VelixHub";
        std::filesystem::create_directories(configPath);
        return (configPath / "config.json").string();
    }
    else
        throw std::runtime_error("Failed to get APPDATA folder");
#else
    const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");

    std::filesystem::path configDir;

    if (xdgConfigHome && strlen(xdgConfigHome) > 0)
        configDir = std::filesystem::path(xdgConfigHome) / "VelixHub";
    else
    {
        const char* homeDir = std::getenv("HOME");
        if (!homeDir)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd)
                homeDir = pwd->pw_dir;
            else
                throw std::runtime_error("Cannot find home directory");
        }
        configDir = std::filesystem::path(homeDir) / ".config" / "VelixHub";
    }

    std::filesystem::create_directories(configDir);

    return (configDir / "config.json").string();
#endif
}
