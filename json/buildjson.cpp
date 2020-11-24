#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "conf.hpp"
#include "json/buildjson.hpp"

namespace conf
{
void from_json(const nlohmann::json& jsonData, conf::SensorConfig& configItem)
{
    jsonData.at("name").get_to(configItem.name);
    jsonData.at("unit").get_to(configItem.unit);
    jsonData.at("type").get_to(configItem.type);
    jsonData.at("path").get_to(configItem.path);

    auto parameters = jsonData.at("parameters");
    parameters.at("type").get_to(configItem.parametersType);
    parameters.at("maxTemp").get_to(configItem.parametersMaxTemp);
    parameters.at("path").get_to(configItem.parametersPath);
    parameters.at("sysLabel").get_to(configItem.parametersSysLabel);
    parameters.at("targetTemp").get_to(configItem.parametersTargetTemp);
    parameters.at("targetTempOffset").get_to(configItem.parametersTargetTempOffset);
    parameters.at("scalar").get_to(configItem.parametersScalar);
}

void from_json(const nlohmann::json& jsonData, conf::ZoneConfig& configItem)
{
    jsonData.at("id").get_to(configItem.id);
    jsonData.at("zoneSetpoint").get_to(configItem.setPoint);
    jsonData.at("target").get_to(configItem.targetPath);
    jsonData.at("components").get_to(configItem.components);
}
}

std::map<std::string, struct conf::SensorConfig>
    getSensorInfo(const nlohmann::json& data)
{
    std::map<std::string, struct conf::SensorConfig> config;
    auto sensors = data["sensors"];

    for (const auto& sensor : sensors)
    {
        config[sensor["name"]] = sensor.get<struct conf::SensorConfig>();
    }

    return config;
}

std::map<int, conf::SkuConfig>
    getSkuInfo(const nlohmann::json& data)
{
    std::map<int, conf::SkuConfig> skusConfig;
    auto skus = data["skus"];

    for (const auto& sku : skus)
    {
        conf::SkuConfig skuZonesInfo;
        auto num = sku["num"];
        auto zones = sku["zones"];

        for (const auto& zone : zones)
        {
            skuZonesInfo[zone["id"]] = zone.get<struct conf::ZoneConfig>();
        }
        skusConfig[num] = skuZonesInfo;
    }

    return skusConfig;
}

void validateJson(const nlohmann::json& data)
{
    if (data.count("sensors") != 1)
    {
        throw "KeyError: 'sensors' not found (or found repeatedly)";
    }

    if (data["sensors"].size() == 0)
    {
        throw "Invalid Configuration: At least one sensor required";
    }

    if (data.count("skus") != 1)
    {
        throw "KeyError: 'skus' not found (or found repeatedly)";
    }

    for (const auto& sku : data["skus"])
    {
        if (sku.count("zones") != 1)
        {
            throw "KeyError: should only have one 'zones' key per sku.";
        }

        if (sku["zones"].size() == 0)
        {
            throw "Invalid Configuration: must be at least one zones per sku.";
        }
    }
}

nlohmann::json parseValidateJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        std::cerr << "Unable to open json file" << std::endl;
        throw;
    }

    auto data = nlohmann::json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        std::cerr << "Invalid json - parse failed" << std::endl;
        throw;
    }

    /* Check the data. */
    validateJson(data);

    return data;
}
