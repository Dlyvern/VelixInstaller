#include "Installer.hpp"

bool Installer::init()
{
    //TODO What SHGetFolderPathA actually returns on windows?
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path)))
    {
        std::filesystem::path installDir = std::filesystem::path(path) / "Velix";
        std::filesystem::create_directories(installDir);
        m_workPath = installDir;
    }
    else
        throw std::runtime_error("Failed to get APPDATA folder");
#else
    const char* homeDir = std::getenv("HOME");

    if (!homeDir)
    {
        if (auto pwd = getpwuid(getuid()))
            homeDir = pwd->pw_dir;
        else
            throw std::runtime_error("Could not find home directory");
    }

    std::filesystem::path installDir = std::filesystem::path(homeDir) / ".local" / "share" / "Velix";

    std::filesystem::create_directories(installDir);

    m_workPath = installDir;
#endif

    return true;
}

std::filesystem::path Installer::getWorkPath() const
{
    return m_workPath;
}