#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "conf.hpp"
#include "json/buildjson.hpp"

namespace conf
{
void from_json(const nlohmann::json& jsonData, conf::sensorConfig& configItem)
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

void from_json(const nlohmann::json& jsonData, std::vector<std::string>& components)
{
    jsonData.at("components").get_to(components);
}
}

std::map<std::string, struct conf::sensorConfig>
    getSensorInfo(const nlohmann::json& data)
{
    std::map<std::string, struct conf::sensorConfig> config;
    auto sensors = data["sensors"];

    for (const auto& sensor : sensors)
    {
        config[sensor["name"]] = sensor.get<struct conf::sensorConfig>();
    }

    return config;
}

conf::skuConfig getSkuInfo(const nlohmann::json& data)
{
    conf::skuConfig skusConfig;
    auto skus = data["skus"];

    for (const auto& sku : skus)
    {
        std::map<int, std::pair<std::pair<int, std::string>, std::vector<std::string>>> skuZonesInfo;
        auto num = sku["num"];
        auto zones = sku["zones"];

        for (const auto& zone : zones)
        {
            auto id = zone["id"];
            auto target = zone["target"];
            auto zoneSetpoint = zone["zoneSetpoint"];
            auto components =
                zone["components"].get<std::vector<std::string>>();

            skuZonesInfo[id].first.first = zoneSetpoint;
            skuZonesInfo[id].first.second = target;
            for (const auto& i : components)
            {
                skuZonesInfo[id].second.push_back(i);
            }
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
