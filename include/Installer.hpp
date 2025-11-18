#ifndef INSTALLER_HPP
#define INSTALLER_HPP

#include <string>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <pwd.h>
#endif


class Installer
{
public:
    bool init();

    std::filesystem::path getWorkPath() const;

private:
    std::filesystem::path m_workPath;
};


#endif //INSTALLER_HPP