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

constexpr auto debugEnablePath = "/etc/thermal.d/margindebug";
extern bool debugEnabled;

constexpr auto spofDisablePath = "/etc/thermal.d/spofdisable";
extern bool spofEnabled;

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

    debugEnabled = false;
    if (std::filesystem::exists(debugEnablePath))
    {
        debugEnabled = true;
        std::cerr << "Debug logging enabled\n";
    }

    // TODO(): This changes default behavior,
    // from fail-if-all to fail-if-one sensor broken,
    // which may be surprising to other users,
    // so consider changing this before releasing.
    spofEnabled = true;
    if (std::filesystem::exists(spofDisablePath))
    {
        spofEnabled = false;
        std::cerr << "Single point of failure disabled\n";
    }

    run(configPath);

    return 0;
}
