#pragma once

#include <string>
#include <vector>
#include <map>

namespace conf
{
struct sensorConfig
{
    std::string name;
    std::string sensorType;
    std::string unit;
    std::string pathType;
    std::string dbusPath;
    std::string sysPath;
    std::string sysLabel;
    std::string sysInput;
    int sysChannel;
    std::string sysReg;
    int offset;

    /** spec info **/
    std::string specType;
    int specTemp;
    std::string specPath;
    std::string specSysLabel;
    std::string specSysInput;
    int specSysChannel;
    std::string specSysReg;
};

using skuConfig = std::map<int, std::map<int, std::vector<std::string>>>;
}
