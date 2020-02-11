#include <iostream>
#include <string>
#include <map>

#include <nlohmann/json.hpp>

#include "conf.hpp"
#include "json/buildjson.hpp"
#include "sensor/sensor.hpp"
#include "util/util.hpp"
#include "dbus/dbus.hpp"

constexpr auto marginConfigPath =
    "/usr/share/read-margin-temp/config-margin.json";

std::map<std::string, struct conf::sensorConfig> sensorConfig = {};
conf::skuConfig skusConfig;

void run()
{
    try
    {
        auto jsonData = parseValidateJson(marginConfigPath);
        sensorConfig = getSensorInfo(jsonData);
        skusConfig = getSkuInfo(jsonData);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed during building json file: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

    /** determine sku **/
    int skuNum = getSkuNum();

    /** get sku info **/
    auto skuConfig = skusConfig[skuNum];

    /** start updating margin temp loop **/
    updateMarginTempLoop(skuConfig, sensorConfig);
}

int main()
{
    run();

    return 0;
}
