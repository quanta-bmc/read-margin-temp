#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <map>

/**
 * Parse sensor config from json data.
 *
 * @param[in] data - json data.
 * @return Json data.
 */
std::map<std::string, struct conf::SensorConfig>
    getSensorInfo(const nlohmann::json& data);

/**
 * Parse sku config from json data.
 *
 * @param[in] data - json data.
 * @return Sku config.
 */
std::map<int, conf::SkuConfig> getSkuInfo(const nlohmann::json& data);

/**
 * Validate json data.
 *
 * @param[in] data - json data.
 */
void validateJson(const nlohmann::json& data);

/**
 * Parse json file.
 *
 * @param[in] path - json file path.
 * @return Json data.
 */
nlohmann::json parseValidateJson(const std::string& path);
