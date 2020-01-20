#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <map>

std::map<std::string, struct conf::sensorConfig>
    getSensorInfo(const nlohmann::json& data);

conf::skuConfig getSkuInfo(const nlohmann::json& data);

void validateJson(const nlohmann::json& data);

nlohmann::json parseValidateJson(const std::string& path);
