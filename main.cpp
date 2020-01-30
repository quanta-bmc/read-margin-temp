#include <iostream>
#include <string>
#include <map>
#include <utility>

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
    std::map<int, std::vector<std::string>> skuConfig;
    int skuNum;

    try
    {
        auto jsonData = parseValidateJson(marginConfigPath);
        sensorConfig = getSensorInfo(jsonData);
        skusConfig = getSkuInfo(jsonData);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed during building: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

    /** determine sku **/
    skuNum = getSkuNum();
    skuConfig = skusConfig[skuNum];

    /** start update loop **/
    updateMarginTempLoop(skuConfig, sensorConfig);
}

int main()
{
    run();

    return 0;
}
