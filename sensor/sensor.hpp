#pragma once

#include <string>

std::string getSensorDeviceAddr(std::string partialPath, std::string reg);

std::string getSensorHwmonNum(std::string partialPath);

std::string getSysPath(std::string path,
    std::string input, int channelNum, std::string reg);
