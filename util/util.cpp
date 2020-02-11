#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <vector>

#include <sdbusplus/server.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>

#include "conf.hpp"
#include "util/util.hpp"
#include "sensor/sensor.hpp"
#include "dbus/dbus.hpp"

int getSkuNum()
{
    /* TO DO:
     * The method of determining sku is not yet known.
     */

    return 1;
}

int getSensorDbusTemp(std::string sensorDbusPath)
{
    auto bus = sdbusplus::bus::new_default();
    std::string service = getService(sensorDbusPath);

    if (service.empty())
    {
        return -1;
    }

    auto temp = dbus::SDBusPlus::getValueProperty(bus, service, sensorDbusPath);

    return temp;
}

int getSpecTemp(struct conf::sensorConfig config)
{
    if (config.spec->specTemp != -1)
    {
        return config.specTemp;
    }

    int specTemp = -1;

    if (config.specType == "sys")
    {
        std::fstream sensorSpecFile;
        std::string path;

        path = getSysPath(config.specSysPath, config.specSysInput, 
            config.specSysChannel, config.specSysReg);
        sensorSpecFile.open(path, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> specTemp;
        }
        
        sensorSpecFile.close();
    }

    return specTemp;
}

std::string getService(const std::string path)
{
    auto bus = sdbusplus::bus::new_system();
    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapper.append(path);
    mapper.append(std::vector<std::string>({"xyz.openbmc_project.Sensor.Value"}));

    std::map<std::string, std::vector<std::string>> response;

    try
    {
        auto responseMsg = bus.call(mapper);
        responseMsg.read(response);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        return "";
    }

    if (response.begin() == response.end())
    {
        return "";
    }

    return response.begin()->first;
}

void updateDbusMarginTemp(int zoneNum, int64_t marginTemp)
{
    auto bus = sdbusplus::bus::new_default();
    std::string service = "xyz.openbmc_project.Hwmon.external";
    std::string path = "/xyz/openbmc_project/extsensors/margin/fleeting";

    path += std::to_string(zoneNum);

    dbus::SDBusPlus::setValueProperty(bus, service, path, marginTemp);
}

void updateMarginTempLoop(
    std::map<int, std::vector<std::string>> skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig)
{
    std::fstream sensorTempFile;
    int numOfZones = skuConfig.size();
    int sensorRealTemp;
    int sensorSpecTemp;
    int sensorMarginTemp;
    int sensorCalibTemp;
    int64_t calibMarginTemp;
    std::map<std::string, struct conf::sensorConfig> sensorList[numOfZones];

    for (int i = 0; i < numOfZones; i++)
    {
        for (auto t = skuConfig[i].begin(); t != skuConfig[i].end(); t++)
        {
            sensorList[i][*t] = sensorConfig[*t];
        }
    }

    while (true)
    {
        for (int i = 0; i < numOfZones; i++)
        {
            calibMarginTemp = 0;
            for (auto t = sensorList[i].begin(); t != sensorList[i].end(); t++)
            {
                sensorRealTemp = 0;
                if (sensorList[i][t->first].spec->specTemp == -1)
                {
                    sensorSpecTemp =
                        getSpecTemp(sensorList[i][t->first]);
                }
                else
                {
                    sensorSpecTemp = sensorList[i][t->first].spec->specTemp;
                }

                if (sensorList[i][t->first].pathType.compare("sys") == 0)
                {
                    std::string path;

                    path = getSysPath(t->second.sysPath,
                        t->second.sysInput, t->second.sysChannel,
                        t->second.sysReg);
                    sensorTempFile.open(path, std::ios::in);
                    if (sensorTempFile)
                    {
                        sensorTempFile >> sensorRealTemp;
                    }
                    else
                    {
                        break;
                    }
                    
                    sensorTempFile.close();
                }
                else if (sensorList[i][t->first].pathType.compare("dbus") == 0)
                {
                    sensorRealTemp = 
                        getSensorDbusTemp(sensorList[i][t->first].dbusPath);
                }

                if (sensorList[i][t->first].unit.compare("degree") == 0)
                {
                    sensorRealTemp *= 1000;
                    sensorSpecTemp *= 1000;
                }

                if (sensorRealTemp != -1)
                {
                    sensorMarginTemp = (sensorSpecTemp - sensorRealTemp);
                    sensorCalibTemp = sensorMarginTemp;
                    sensorCalibTemp += sensorList[i][t->first].offset;

                    if (calibMarginTemp == 0 ||
                        sensorMarginTemp < calibMarginTemp)
                    {
                        calibMarginTemp = sensorCalibTemp;
                    }
                }
            }

            updateDbusMarginTemp(i, calibMarginTemp);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
