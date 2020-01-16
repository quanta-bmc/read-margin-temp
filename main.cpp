#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <utility>

#include <nlohmann/json.hpp>

#include "conf.hpp"
#include "json/buildjson.hpp"
#include "sensor/sensor.hpp"
#include "util/util.hpp"

#define FAN_TABLE_SKU1_FILE "/usr/share/swampd/config-sku1.json"
#define FAN_TABLE_SKU2_FILE "/usr/share/swampd/config-sku2.json"
#define FAN_TABLE_SKU3_FILE "/usr/share/swampd/config-sku3.json"
#define FAN_TABLE_FILE "/usr/share/swampd/config.json"
#define FAN_PATH "/sys/devices/platform/ahb/ahb:apb/f0083000.i2c/i2c-3/3-0075"
#define CPU_PATH "/sys/devices/platform/ahb/ahb:apb/f0100000.peci-bus/peci-0"

constexpr auto marginConfigPath = 
    "/usr/share/read-margin-temp/config-margin.json";

std::map<std::string, struct conf::sensorConfig> sensorConfig;
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

int main(int argc, char* argv[])
{
    run();

    return 0;
}
