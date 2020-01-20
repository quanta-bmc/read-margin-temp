#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <utility>

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server/manager.hpp>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include "conf.hpp"
#include "json/buildjson.hpp"
#include "sensor/sensor.hpp"
#include "util/util.hpp"
#include "dbus/dbus.hpp"

constexpr auto marginConfigPath =
    "/usr/share/read-margin-temp/config-margin.json";

constexpr auto marginTempRequestName = "xyz.openbmc_project.Hwmon.external";

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

int main()
{
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();

    std::string INVENTORY_BUSNAME = "xyz.openbmc_project.Hwmon.external";

    std::string ITEM_IFACE = "xyz.openbmc_project.Sensor.Value";

    std::string path = "/xyz/openbmc_project/extsensors/margin/fleeting0";

    int64_t value = 10;

    dbus::SDBusPlus::setProperty(
        bus, INVENTORY_BUSNAME, path, ITEM_IFACE, "Value", value);

    run();

    return 0;
}
