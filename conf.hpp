#pragma once

#include <string>
#include <vector>

namespace conf
{
struct sensorConfig
{
    int exponent;
    std::string pathType;
    std::string upperPath;
    std::string lowerPath;
    int channel;
    std::string reg;
    int spec;
};

using skuConfig = std::map<int, std::vector<std::string>>;
} // namespace conf
