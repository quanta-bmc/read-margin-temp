#pragma once

#include <map>
#include <vector>
#include <string>

#include "conf.hpp"

int getSkuNum();

int getSensorDbusTemp(std::string busName, std::string sensorDbusPath);

void updateDbusMarginTemp(int zoneNum, int64_t marginTemp);

void updateMarginTempLoop(
    std::map<int, std::vector<std::string>> skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig);
