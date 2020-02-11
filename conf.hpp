#pragma once

#include <string>
#include <vector>
#include <map>

namespace conf
{
struct specInfo
{
    std::string type;
    int specTemp;
    std::string sysPath;
    std::string sysInput;
    int sysChannel;
    std::string sysReg;
};

struct sensorConfig
{
    std::string name;
    std::string sensorType;
    std::string unit;
    std::string pathType;
    std::string dbusPath;
    std::string sysPath;
    std::string sysInput;
    int sysChannel;
    std::string sysReg;
    int offset;
    specInfo* spec;
};

using skuConfig = std::map<int, std::map<int, std::vector<std::string>>>;
}
