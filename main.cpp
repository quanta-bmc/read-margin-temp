#include <iostream>
#include <string>
#include <map>

#include <nlohmann/json.hpp>

#include "conf.hpp"
#include "json/buildjson.hpp"
#include "sensor/sensor.hpp"
#include "util/util.hpp"
#include "dbus/dbus.hpp"

constexpr auto MARGINCONFIGPATH =
    "/usr/share/read-margin-temp/config-margin.json";

std::map<std::string, struct conf::SensorConfig> sensorConfig = {};
std::map<int, conf::SkuConfig> skusConfig;

void run(const std::string& configPath)
{
    try
    {
        auto jsonData = parseValidateJson(configPath);
        sensorConfig = getSensorInfo(jsonData);
        skusConfig = getSkuInfo(jsonData);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed during building json file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    /** determine sku **/
    int skuNum = getSkuNum();

    /** get sku info **/
    auto skuConfig = skusConfig[skuNum];

    /** start updating margin temp loop **/
    updateMarginTempLoop(skuConfig, sensorConfig);
}

int main(int argc, char **argv)
{
    std::string configPath = MARGINCONFIGPATH;

    if (argc > 1)
    {
        configPath = argv[1];
    }

    run(configPath);

    return 0;
}
