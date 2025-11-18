#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "json/json.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <pwd.h>
#endif

class Config
{
public:
    bool load(const std::string& path = "");

    nlohmann::json getConfig();

private:
    std::string getConfigPath();

    nlohmann::json m_config;
};


#endif //CONFIG_HPP