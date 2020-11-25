#pragma once

#include <string>

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
 * @param[in] label - label name.
 * @return Path.
 */
std::string getSysPath(std::string path, std::string label = "");
