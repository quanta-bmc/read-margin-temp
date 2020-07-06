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
    /**
     * TO DO:
     * The method of determining sku is not yet known. Now default sku number
     * is 1.
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

    return (int)(temp * 1000);
}

int getSpecTemp(struct conf::sensorConfig config)
{
    if (config.parametersMaxTemp != -1)
    {
        return config.parametersMaxTemp;
    }

    int specTemp = -1;
    std::fstream sensorSpecFile;

    if (config.parametersType == "sys")
    {
        std::string path;

        path = getSysPath(config.parametersPath, config.parametersSysLabel);
        sensorSpecFile.open(path, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> specTemp;
        }

        sensorSpecFile.close();
    }
    else if (config.parametersType == "file")
    {
        sensorSpecFile.open(config.parametersPath, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> specTemp;
        }

        sensorSpecFile.close();
    }

    return specTemp;
}

int calOffsetValue(int setPoint, double scalar, int maxTemp, int targetTemp, int targetOffset)
{
    int offsetvalue = 0;
    offsetvalue = setPoint / scalar;
    if (targetTemp == -1)
    {
        targetTemp = maxTemp;
    }
    offsetvalue -= maxTemp - ( targetTemp + targetOffset );
    return offsetvalue;
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

void updateDbusMarginTemp(int zoneNum, int64_t marginTemp, std::string targetpath)
{
    auto bus = sdbusplus::bus::new_default();
    std::string service = "xyz.openbmc_project.Hwmon.external";

    dbus::SDBusPlus::setValueProperty(bus, service, targetpath, marginTemp);
}

void updateMarginTempLoop(
    std::map<int, std::pair<std::pair<int, std::string>, std::vector<std::string>>> skuConfig,
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
        for (auto t = skuConfig[i].second.begin(); t != skuConfig[i].second.end(); t++)
        {
            sensorList[i][*t] = sensorConfig[*t];
        }
    }

    while (true)
    {
        for (int i = 0; i < numOfZones; i++)
        {
            calibMarginTemp = -1;
            for (auto t = sensorList[i].begin(); t != sensorList[i].end(); t++)
            {
                sensorRealTemp = 0;
                if (sensorList[i][t->first].parametersMaxTemp == -1)
                {
                    sensorSpecTemp =
                        getSpecTemp(sensorList[i][t->first]);
                }
                else
                {
                    sensorSpecTemp = sensorList[i][t->first].parametersMaxTemp;
                }

                if (sensorList[i][t->first].type == "sys")
                {
                    std::string path;

                    path = getSysPath(t->second.path);
                    sensorTempFile.open(path, std::ios::in);
                    if (sensorTempFile)
                    {
                        sensorTempFile >> sensorRealTemp;
                    }
                    else
                    {
                        sensorTempFile.close();
                        continue;
                    }
                    
                    sensorTempFile.close();
                }
                else if (sensorList[i][t->first].type == "dbus")
                {
                    sensorRealTemp = 
                        getSensorDbusTemp(sensorList[i][t->first].path);
                }
                else if (sensorList[i][t->first].type == "file")
                {
                    std::fstream sensorValueFile;
                    sensorValueFile.open(sensorList[i][t->first].path, std::ios::in);
                    if (sensorValueFile)
                    {
                        sensorValueFile >> sensorRealTemp;
                    }

                    sensorValueFile.close();
                }

                if (sensorList[i][t->first].unit == "degree")
                {
                    sensorRealTemp *= 1000;
                    sensorSpecTemp *= 1000;
                }
                else if (sensorList[i][t->first].unit == "millimargin"){
                    if (calibMarginTemp == -1 ||
                        sensorRealTemp < calibMarginTemp)
                    {
                        calibMarginTemp = sensorRealTemp;
                    }
                    continue;
                }

                if (sensorRealTemp != -1)
                {
                    sensorMarginTemp = (sensorSpecTemp - sensorRealTemp);
                    sensorCalibTemp = sensorMarginTemp;
                    auto offsetVal = calOffsetValue(skuConfig[i].first.first,
                                                    sensorList[i][t->first].parametersScalar,
                                                    sensorSpecTemp,
                                                    sensorList[i][t->first].parametersTargetTemp,
                                                    sensorList[i][t->first].parametersTargetTempOffset);
                    sensorCalibTemp += offsetVal;
                    sensorCalibTemp *= sensorList[i][t->first].parametersScalar;

                    if (calibMarginTemp == -1 ||
                        sensorCalibTemp < calibMarginTemp)
                    {
                        calibMarginTemp = sensorCalibTemp;
                    }
                }
            }

            updateDbusMarginTemp(i, calibMarginTemp, skuConfig[i].first.second);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
