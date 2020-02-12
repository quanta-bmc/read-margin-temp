#pragma once

#include <string>

/**
 * Get device address.
 *
 * @param[in] partialPath - partial path.
 * @param[in] reg - register name.
 * @return Sku config.
 */
std::string getSensorDeviceAddr(std::string partialPath, std::string reg);

/**
 * Get hwmon number.
 *
 * @param[in] partialPath - partial path.
 * @return String "hwmon<num>".
 */
std::string getSensorHwmonNum(std::string partialPath);

/**
 * Get sys path.
 *
 * @param[in] path - partial sys path.
 * @param[in] input - input name.
 * @param[in] channelNum - channel number.
 * @param[in] reg - register number.
 * @return Path.
 */
std::string getSysPath(std::string path,
    std::string input, int channelNum, std::string reg);
