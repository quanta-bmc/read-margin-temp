#pragma once

#include <string>
#include <vector>
#include <map>

namespace conf
{
struct SensorConfig
{
    std::string name;
    std::string unit;
    std::string type;
    std::string path;

    /** parameters info **/
    std::string parametersType;
    int parametersMaxTemp;
    std::string parametersPath;
    std::string parametersSysLabel;
    int parametersTargetTemp;
    int parametersTargetTempOffset;
    double parametersScalar;
};

struct ZoneConfig
{
    int id;
    int setPoint;
    std::string targetPath;
    std::vector<std::string> components;
};

using SkuConfig = std::map<int, conf::ZoneConfig>;
}
