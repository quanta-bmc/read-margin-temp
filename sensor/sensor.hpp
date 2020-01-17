#pragma once

#include <string>

std::string getSensorDeviceAddr(std::string partialPath, std::string reg);

std::string getSensorHwmonNum(std::string partialPath);

std::string getSensorPath(struct conf::sensorConfig sensorConfig);
