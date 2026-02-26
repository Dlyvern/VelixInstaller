#ifndef PROJECT_DATA_HPP
#define PROJECT_DATA_HPP

#include <string>

//this namespace is kinda dummy
namespace project
{
    struct ProjectData
    {
    public:
        std::string name;
        std::string path;
        std::string projectFilePath;
    };
} //namespace project

#endif //PROJECT_DATA_HPP
