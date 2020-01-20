#pragma once

#include <map>
#include <vector>

#include "conf.hpp"

int getSkuNum();

void updateDbusMarginTemp(int64_t marginTemp);

void updateMarginTempLoop(
    std::map<int, std::vector<std::string>> skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig);
