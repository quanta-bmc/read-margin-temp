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
    int skuNum = 1;

    /* TBD */

    return skuNum;
}

int getSensorDbusTemp(std::string sensorDbusPath)
{
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    std::string service = getService(sensorDbusPath);
    std::string itemIface = "xyz.openbmc_project.Sensor.Value";

    if (service.empty())
    {
        return -1;
    }

    auto temp = dbus::SDBusPlus::getProperty(
        bus, service, sensorDbusPath, itemIface, "Value");

    return temp;
}

int getSpecTemp()
{
    int specTemp = 0;

    /* TBD */

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
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    std::string service = "xyz.openbmc_project.Hwmon.external";
    std::string itemIface = "xyz.openbmc_project.Sensor.Value";
    std::string path = "/xyz/openbmc_project/extsensors/margin/fleeting";

    path += std::to_string(zoneNum);

    dbus::SDBusPlus::setProperty(
        bus, service, path, itemIface, "Value", marginTemp);
}

void updateMarginTempLoop(
    std::map<int, std::vector<std::string>> skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig)
{
    std::fstream sensorTempFile;
    std::string tmp;
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
                if (sensorList[i][t->first].spec == -1)
                {
                    sensorSpecTemp = getSpecTemp();
                }
                else
                {
                    sensorSpecTemp = sensorList[i][t->first].spec;
                }

                if (sensorList[i][t->first].pathType.compare("sys") == 0)
                {
                    sensorTempFile.open(getSensorPath(t->second), std::ios::in);
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
                        getSensorDbusTemp(sensorList[i][t->first].path);
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
