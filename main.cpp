#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <utility>

#include <nlohmann/json.hpp>

#include "json/buildjson.hpp"
#include "sensor/sensor.hpp"
#include "util/util.hpp"

using json = nlohmann::json;

#define FAN_TABLE_SKU1_FILE "/usr/share/swampd/config-sku1.json"
#define FAN_TABLE_SKU2_FILE "/usr/share/swampd/config-sku2.json"
#define FAN_TABLE_SKU3_FILE "/usr/share/swampd/config-sku3.json"
#define FAN_TABLE_FILE "/usr/share/swampd/config.json"
#define FAN_PATH "/sys/devices/platform/ahb/ahb:apb/f0083000.i2c/i2c-3/3-0075"
#define CPU_PATH "/sys/devices/platform/ahb/ahb:apb/f0100000.peci-bus/peci-0"

constexpr auto marginConfigPath = 
    "/usr/share/read-margin-temp/config-margin.json";

std::map<std::string, struct conf::sensorConfig> sensorConfig = {};
conf::skuConfig skuConfig;

void run()
{
    /** determine sku **/
    int skuNum = getSkuNum();

    try
    {
        auto jsonData = parseValidateJson(marginConfigPath);
        sensorConfig = getSensorInfo(jsonData);
        skuConfig = getSkuInfo(jsonData);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed during building: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

    // mgmr = buildSensors(sensorConfig, passiveBus, hostBus);
    // zones = buildZones(zoneConfig, zoneDetailsConfig, mgmr, modeControlBus);

    // if (0 == zones.size())
    // {
    //     std::cerr << "No zones defined, exiting.\n";
    //     std::exit(EXIT_FAILURE);
    // }

    // for (const auto& i : zones)
    // {
    //     auto& timer = timers.emplace_back(io);
    //     std::cerr << "pushing zone " << i.first << "\n";
    //     pidControlLoop(i.second.get(), timer);
    // }
}

int main(int argc, char* argv[])
{
    run();

    return 0;
}
