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
    jsonData.at("sensorType").get_to(configItem.sensorType);
    jsonData.at("unit").get_to(configItem.unit);
    jsonData.at("pathType").get_to(configItem.pathType);
    jsonData.at("dbusPath").get_to(configItem.dbusPath);
    jsonData.at("sysPath").get_to(configItem.sysPath);
    jsonData.at("sysLabel").get_to(configItem.sysLabel);
    jsonData.at("sysInput").get_to(configItem.sysInput);
    jsonData.at("sysChannel").get_to(configItem.sysChannel);
    jsonData.at("sysReg").get_to(configItem.sysReg);
    jsonData.at("offset").get_to(configItem.offset);

    auto spec = jsonData.at("spec");
    spec.at("type").get_to(configItem.specType);
    spec.at("specTemp").get_to(configItem.specTemp);
    spec.at("path").get_to(configItem.specPath);
    spec.at("sysLabel").get_to(configItem.specSysLabel);
    spec.at("sysInput").get_to(configItem.specSysInput);
    spec.at("sysChannel").get_to(configItem.specSysChannel);
    spec.at("sysReg").get_to(configItem.specSysReg);
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
        std::map<int, std::vector<std::string>> skuZonesInfo;
        auto num = sku["num"];
        auto zones = sku["zones"];

        for (const auto& zone : zones)
        {
            auto id = zone["id"];
            auto components =
                zone["components"].get<std::vector<std::string>>();

            for (const auto& i : components)
            {
                skuZonesInfo[id].push_back(i);
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
