#pragma once

#include <string>
#include <vector>
#include <map>

namespace conf
{
struct sensorConfig
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

struct zoneConfig
{
    int id;
    int setpoint;
    std::string targetPath;
    std::vector<std::string> components;
};

using skuConfig = std::map<int, conf::zoneConfig>;
}
