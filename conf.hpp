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

using skuConfig = std::map<int, std::map<int, std::pair<std::pair<int, std::string>, std::vector<std::string>>>>;
}
