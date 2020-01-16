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
    jsonData.at("exponent").get_to(configItem.exponent);
    jsonData.at("pathType").get_to(configItem.pathType);
    jsonData.at("upperPath").get_to(configItem.upperPath);
    jsonData.at("lowerPath").get_to(configItem.lowerPath);
    jsonData.at("channel").get_to(configItem.channel);
    jsonData.at("reg").get_to(configItem.reg);
    jsonData.at("spec").get_to(configItem.spec);
}

void from_json(
    const nlohmann::json& jsonData, std::vector<std::string>& components)
{
    jsonData.at("components").get_to(components);
}
}

std::map<std::string, struct conf::sensorConfig>
    getSensorInfo(const nlohmann::json& data)
{
    std::map<std::string, struct conf::sensorConfig> config;
    auto sensors = data["sensors"];

    /* TODO: If no sensors, this is invalid, and we should except here or during
     * parsing.
     */
    for (const auto& sensor : sensors)
    {
        config[sensor["name"]] = sensor.get<struct conf::sensorConfig>();
    }

    return config;
}

conf::skuConfig getSkuInfo(const nlohmann::json& data)
{
    conf::skuConfig skuConfig;
    auto skus = data["skus"];
    for (const auto& sku : skus)
    {
        // int num = sku["num"];
        auto zones = sku["zones"];

        for (const auto& zone : zones)
        {
            auto id = zone["id"];
            auto components = zone.get<std::vector<std::string>>();
            skuConfig[id] = components;
        }
    }

    return skuConfig;
}

void validateJson(const nlohmann::json& data)
{
    // if (data.count("sensors") != 1)
    // {
    //     throw ConfigurationException(
    //         "KeyError: 'sensors' not found (or found repeatedly)");
    // }

    // if (data["sensors"].size() == 0)
    // {
    //     throw ConfigurationException(
    //         "Invalid Configuration: At least one sensor required");
    // }

    // if (data.count("skus") != 1)
    // {
    //     throw ConfigurationException(
    //         "KeyError: 'skus' not found (or found repeatedly)");
    // }

    // for (const auto& sku : data["skus"])
    // {
    //     if (sku.count("zones") != 1)
    //     {
    //         throw ConfigurationException(
    //             "KeyError: should only have one 'zones' key per sku.");
    //     }

    //     if (sku["zones"].size() == 0)
    //     {
    //         throw ConfigurationException(
    //             "Invalid Configuration: must be at least one zones per sku.");
    //     }
    // }
}

nlohmann::json parseValidateJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        std::cerr << "Unable to open json file" << std::endl;
    }

    auto data = nlohmann::json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        std::cerr << "Invalid json - parse failed" << std::endl;
    }

    /* Check the data. */
    validateJson(data);

    return data;
}
