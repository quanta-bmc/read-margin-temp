#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <vector>
#include <iostream>

#include <sdbusplus/server.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>

#include "conf.hpp"
#include "util/util.hpp"
#include "sensor/sensor.hpp"
#include "dbus/dbus.hpp"

// Enables logging of margin decisions
static constexpr bool DEBUG = false;

int getSkuNum()
{
    /**
     * TO DO:
     * The method of determining sku is not yet known. Now default sku number
     * is 1.
     */

    return 1;
}

double getSensorDbusTemp(std::string sensorDbusPath, bool unitMilli)
{
    auto bus = sdbusplus::bus::new_default();
    std::string service = getService(sensorDbusPath);

    double value = std::numeric_limits<double>::quiet_NaN();

    if (service.empty())
    {
        std::cerr << "Sensor input path not mappable to service: " << sensorDbusPath << std::endl;
        return value;
    }

    value = dbus::SDBusPlus::getValueProperty(bus, service, sensorDbusPath, unitMilli);
    return value;
}

// As per documentation, specTemp unit, in config or filesystem, is always millidegrees
// However, this function returns degrees
double getSpecTemp(struct conf::sensorConfig config)
{
    double specTemp = std::numeric_limits<double>::quiet_NaN();

    // As per documentation, -1 indicates "value comes from outside the config"
    if (config.parametersMaxTemp != -1)
    {
        // The value is within the config itself, use it as-is
        specTemp = static_cast<double>(config.parametersMaxTemp);

        // Convert millidegrees to degrees
        specTemp /= 1000.0;
        return specTemp;
    }

    std::fstream sensorSpecFile;

    if (config.parametersType == "sys")
    {
        std::string path;

        path = getSysPath(config.parametersPath, config.parametersSysLabel);
        sensorSpecFile.open(path, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> specTemp;
            sensorSpecFile.close();
        }
    }
    else if (config.parametersType == "file")
    {
        sensorSpecFile.open(config.parametersPath, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> specTemp;
            sensorSpecFile.close();
        }
    }

    if (std::isnan(specTemp))
    {
        std::cerr << "Sensor MaxTemp reading not available: " << config.parametersPath << std::endl;
    }
    else
    {
        // Convert millidegrees to degrees
        specTemp /= 1000.0;
    }

    return specTemp;
}

// Avoid losing precision by doing calculations as double
double calOffsetValue(int setPointInt, double scalar, double maxTemp, int targetTempInt, int targetOffsetInt)
{
    // All integers are in millidegrees, convert to degrees
    double setPoint = static_cast<double>(setPointInt);
    setPoint /= 1000.0;
    double targetOffset = static_cast<double>(targetOffsetInt);
    targetOffset /= 1000.0;

    double offsetvalue = 0.0;
    offsetvalue = setPoint / scalar;

    // If targetTemp not specified, use maxTemp instead
    double targetTemp;
    if (targetTempInt == -1)
    {
        targetTemp = maxTemp;
    }
    else
    {
        targetTemp = static_cast<double>(targetTempInt);
        targetTemp /= 1000.0;
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
        std::cerr << "Get dbus service fail. " << ex.what() << std::endl;
        return "";
    }

    if (response.begin() == response.end())
    {
        std::cerr << "Sensor service not found: " << path << std::endl;
        return "";
    }

    return response.begin()->first;
}

void updateDbusMarginTemp(int zoneNum, double marginTemp, std::string targetpath)
{
    auto bus = sdbusplus::bus::new_default();
    std::string service = getService(targetpath);

    if (service.empty())
    {
        std::cerr << "Sensor output path not mappable to service: " << targetpath << std::endl;
        return;
    }

    // The final computed margin output is always in degrees
    dbus::SDBusPlus::setValueProperty(bus, service, targetpath, marginTemp, false);
}

void updateMarginTempLoop(
    conf::skuConfig skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig)
{
    std::fstream sensorTempFile;
    int numOfZones = skuConfig.size();
    double sensorRealTemp;
    double sensorSpecTemp;
    double sensorMarginTemp;
    double sensorCalibTemp;
    double calibMarginTemp;
    std::map<std::string, struct conf::sensorConfig> sensorList[numOfZones];

    for (int i = 0; i < numOfZones; i++)
    {
        for (auto t = skuConfig[i].components.begin(); t != skuConfig[i].components.end(); t++)
        {
            sensorList[i][*t] = sensorConfig[*t];
        }
    }

    while (true)
    {
        for (int i = 0; i < numOfZones; i++)
        {
            // Begin a new zone line of space-separated sensors
            if constexpr (DEBUG)
            {
                // Get the map key at the Nth position within the map
                auto t = skuConfig.begin();
                for (int j = 0; j < i; ++j)
                {
                    // Unfortunately, no operator+=, can't just do t += i
                    ++t;
                }

                std::cerr << "Margin Zone " << t->first << ":";
            }

            // Standardize on floating-point degrees for all computations
            calibMarginTemp = std::numeric_limits<double>::quiet_NaN();

            for (auto t = sensorList[i].begin(); t != sensorList[i].end(); t++)
            {
                // Numbers from D-Bus or filesystem need to know their units
                bool incomingMilli = false;
                if ((sensorList[i][t->first].unit == "millidegree") || (sensorList[i][t->first].unit == "millimargin"))
                {
                   incomingMilli = true;
                }

                // Also need to know if units are absolute or margin
                bool incomingMargin = false;
                if ((sensorList[i][t->first].unit == "margin") || (sensorList[i][t->first].unit == "millimargin"))
                {
                    incomingMargin = true;
                }

                sensorRealTemp = std::numeric_limits<double>::quiet_NaN();

                // This function already returns degrees
                sensorSpecTemp =
                    getSpecTemp(sensorList[i][t->first]);

                if (sensorList[i][t->first].type == "dbus")
                {
                    // This function already returns degrees
                    sensorRealTemp =
                        getSensorDbusTemp(sensorList[i][t->first].path, incomingMilli);
                }
                else
                {
                    if (sensorList[i][t->first].type == "sys")
                    {
                        std::string path;

                        path = getSysPath(t->second.path);
                        sensorTempFile.open(path, std::ios::in);
                        if (sensorTempFile)
                        {
                            sensorTempFile >> sensorRealTemp;
                            sensorTempFile.close();
                        }
                    }
                    else if (sensorList[i][t->first].type == "file")
                    {
                        std::fstream sensorValueFile;
                        sensorValueFile.open(sensorList[i][t->first].path, std::ios::in);
                        if (sensorValueFile)
                        {
                            sensorValueFile >> sensorRealTemp;
                            sensorValueFile.close();
                        }
                    }

                    if (!(std::isnan(sensorRealTemp)))
                    {
                        // If configured unit not already degrees, convert to degrees
                        if (incomingMilli)
                        {
                            sensorRealTemp /= 1000.0;
                        }
                    }
                }

                if (std::isnan(sensorRealTemp))
                {
                    // Sensor failure, unable to get reading
                    if constexpr (DEBUG)
                    {
                        std::cerr << " ?";
                    }

                    continue;
                }

                // If sensor already in margin, then accept it as-is
                if (incomingMargin)
                {
                    // Remember this margin if it is the worst margin
                    if ((std::isnan(calibMarginTemp)) ||
                        sensorRealTemp < calibMarginTemp)
                    {
                        calibMarginTemp = sensorRealTemp;
                    }

                    // Sensor successful, already in margin format
                    if constexpr (DEBUG)
                    {
                        std::cerr << " " << sensorRealTemp;
                    }

                    continue;
                }

                if (std::isnan(sensorSpecTemp))
                {
                    // Sensor failure, needed to know sensorSpecTemp to compute margin
                    if constexpr (DEBUG)
                    {
                        std::cerr << " ?";
                    }

                    continue;
                }

                if (sensorList[i][t->first].parametersScalar == 0)
                {
                    sensorCalibTemp = static_cast<double>(skuConfig[i].setpoint) / 1000.0;
                }
                else
                {
                    // Subtract to compute margin
                    sensorMarginTemp = (sensorSpecTemp - sensorRealTemp);
                    sensorCalibTemp = sensorMarginTemp;

                    // The first parameter: this is the setPoint, integer millidegrees
                    // parametersScalar: floating-point, this is unitless
                    // sensorSpecTemp: floating-point degrees
                    // parametersTargetTemp and TargetTempOffset: integer millidegrees
                    auto offsetVal = calOffsetValue(skuConfig[i].setpoint,
                                                    sensorList[i][t->first].parametersScalar,
                                                    sensorSpecTemp,
                                                    sensorList[i][t->first].parametersTargetTemp,
                                                    sensorList[i][t->first].parametersTargetTempOffset);
                    sensorCalibTemp += offsetVal;
                    sensorCalibTemp *= sensorList[i][t->first].parametersScalar;
                }

                // Remember this margin if it is the worst margin
                if ((std::isnan(calibMarginTemp)) ||
                    sensorCalibTemp < calibMarginTemp)
                {
                    calibMarginTemp = sensorCalibTemp;
                }

                // Sensor successful, converted absolute to margin
                if constexpr (DEBUG)
                {
                    std::cerr << " " << sensorCalibTemp;
                }
            }

            // If all sensors failed, conservatively assume we have no margin
            if (std::isnan(calibMarginTemp))
            {
                calibMarginTemp = 0;
            }

            updateDbusMarginTemp(i, calibMarginTemp, skuConfig[i].targetPath);

            // Finish sensor line, indicate computed worst margin
            if constexpr (DEBUG)
            {
                std::cerr << " => " << calibMarginTemp << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
