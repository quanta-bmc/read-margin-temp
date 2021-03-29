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

extern sdbusplus::bus::bus *g_system_bus, *g_default_bus;

// Enables single-point-of-failure behavior
bool spofEnabled = false;

// Enables logging of margin decisions
bool debugEnabled = false;

// Enable ignore empty sensor service failure
bool ignoreEnable = false;

// Enable consider NVMe sensor Present property
bool nvmePresentEnable = true;

// flag when sensor service is empty
bool emptyService = false;

int getSkuNum()
{
    /**
     * TO DO:
     * The method of determining sku is not yet known. Now default sku number
     * is 1.
     */

    return 1;
}

double readDoubleOrNan(std::istream& is)
{
    double value = std::numeric_limits<double>::quiet_NaN();

    // Consume stream until whitespace or end encountered
    std::string text;
    is >> text;

    // Use idx for error detection
    size_t idx;
    double parsed = std::stod(text, &idx);

    // Ensure string non-empty and entire string length parsed
    if ( (idx > 0) && (idx == text.size()) )
    {
        value = parsed;
    }

    return value;
}

double getSensorDbusTemp(std::string sensorDbusPath, bool unitMilli)
{
    sdbusplus::bus::bus& bus = *g_default_bus;
    std::string service = getService(sensorDbusPath, VALUEINTERFACE);

    double value = std::numeric_limits<double>::quiet_NaN();

    // check NVMe sensor Present property
    if (sensorDbusPath.find("nvme") != std::string::npos && nvmePresentEnable)
    {
        std::string nvmeInventoryPath = sensorDbusPath;
        nvmeInventoryPath.erase(nvmeInventoryPath.begin(),
            nvmeInventoryPath.begin() + nvmeInventoryPath.find("nvme"));
        nvmeInventoryPath =
            "/xyz/openbmc_project/inventory/system/chassis/motherboard/" + nvmeInventoryPath;
        std::string nvmeService = getService(nvmeInventoryPath, PRESENTINTERFACE);

        if (!dbus::SDBusPlus::checkNvmePresentProperty(bus,
                                                       nvmeService,
                                                       nvmeInventoryPath))
        {
            // Treat missing NVMe similarly to other missing services
            if (ignoreEnable)
            {
                emptyService = true;
            }
            return value;
        }
    }

    if (service.empty())
    {
        if (ignoreEnable)
        {
            emptyService = true;
        }
        // std::cerr << "Sensor input path not mappable to service: " << sensorDbusPath << std::endl;
        return value;
    }

    if (spofEnabled)
    {
        // If sensor functional property is false, set NaN to dbus and return NaN.
        if (!dbus::SDBusPlus::checkFunctionalProperty(bus,
                                                      service,
                                                      sensorDbusPath))
        {
            return value;
        }

        if (dbus::SDBusPlus::checkWarningProperty(bus,
                                                  service,
                                                  sensorDbusPath,
                                                  "WarningAlarmHigh"))
        {
            return value;
        }
        if (dbus::SDBusPlus::checkWarningProperty(bus,
                                                  service,
                                                  sensorDbusPath,
                                                  "WarningAlarmLow"))
        {
            return value;
        }

        if (dbus::SDBusPlus::checkCriticalProperty(bus,
                                                   service,
                                                   sensorDbusPath,
                                                   "CriticalAlarmHigh"))
        {
            return value;
        }
        if (dbus::SDBusPlus::checkCriticalProperty(bus,
                                                   service,
                                                   sensorDbusPath,
                                                   "CriticalAlarmLow"))
        {
            return value;
        }
    }

    value = dbus::SDBusPlus::getValueProperty(bus,
                                              service,
                                              sensorDbusPath,
                                              unitMilli);

    return value;
}

// As per documentation, specTemp unit, in config or filesystem, is always millidegrees
// However, this function returns degrees
double getSpecTemp(struct conf::SensorConfig config)
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
            specTemp = readDoubleOrNan(sensorSpecFile);
            sensorSpecFile.close();
        }
    }
    else if (config.parametersType == "file")
    {
        sensorSpecFile.open(config.parametersPath, std::ios::in);
        if (sensorSpecFile)
        {
            specTemp = readDoubleOrNan(sensorSpecFile);
            sensorSpecFile.close();
        }
    }

    if (!(std::isfinite(specTemp)))
    {
        // std::cerr << "Sensor MaxTemp reading not available: " << config.parametersPath << std::endl;
    }
    else
    {
        // Convert millidegrees to degrees
        specTemp /= 1000.0;
    }

    return specTemp;
}

// Avoid losing precision by doing calculations as double
double calOffsetValue(int setPointInt,
                      double scalar,
                      double maxTemp,
                      int targetTempInt,
                      int targetOffsetInt)
{
    // All integers are in millidegrees, convert to degrees
    double setPoint = static_cast<double>(setPointInt);
    setPoint /= 1000.0;
    double targetOffset = static_cast<double>(targetOffsetInt);
    targetOffset /= 1000.0;

    double offsetValue = 0.0;
    offsetValue = setPoint / scalar;

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

    offsetValue -= maxTemp - ( targetTemp + targetOffset );
    return offsetValue;
}

std::string getService(const std::string dbusPath, std::string interfacePath)
{
    sdbusplus::bus::bus& bus = *g_system_bus;
    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapper.append(dbusPath);
    mapper.append(std::vector<std::string>({interfacePath}));

    std::map<std::string, std::vector<std::string>> response;

    try
    {
        auto responseMsg = bus.call(mapper);
        responseMsg.read(response);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        // std::cerr << "Get dbus service fail. " << ex.what() << std::endl;
        return "";
    }

    if (response.begin() == response.end())
    {
        // std::cerr << "Sensor service not found: " << dbusPath << std::endl;
        return "";
    }

    return response.begin()->first;
}

void updateDbusMarginTemp(int zoneNum,
                          double marginTemp,
                          std::string targetPath)
{
    sdbusplus::bus::bus& bus = *g_default_bus;
	std::string service = getService(targetPath, VALUEINTERFACE);

    if (service.empty())
    {
        // std::cerr << "Sensor output path not mappable to service: " << targetpath << std::endl;
        return;
    }

    // The final computed margin output is always in degrees
    dbus::SDBusPlus::setValueProperty(bus,
                                      service,
                                      targetPath,
                                      marginTemp,
                                      false);
}

void updateMarginTempLoop(
        conf::SkuConfig skuConfig,
        std::map<std::string, struct conf::SensorConfig> sensorConfig)
{
    std::fstream sensorTempFile;
    int numOfZones = skuConfig.size();
    double sensorRealTemp;
    double sensorSpecTemp;
    double sensorMarginTemp;
    double sensorCalibTemp;
    double calibMarginTemp;
    std::map<std::string, struct conf::SensorConfig> sensorList[numOfZones];

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
            if (debugEnabled)
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

            bool errorEncountered = false;

            // Standardize on floating-point degrees for all computations
            calibMarginTemp = std::numeric_limits<double>::quiet_NaN();

            for (auto t = sensorList[i].begin(); t != sensorList[i].end(); t++)
            {
                emptyService = false;

                // Numbers from D-Bus or filesystem need to know their units
                bool incomingMilli = false;
                if (sensorList[i][t->first].unit == "millidegree" ||
                    sensorList[i][t->first].unit == "millimargin")
                {
                   incomingMilli = true;
                }

                // Also need to know if units are absolute or margin
                bool incomingMargin = false;
                if (sensorList[i][t->first].unit == "margin" ||
                    sensorList[i][t->first].unit == "millimargin")
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
                        getSensorDbusTemp(sensorList[i][t->first].path,
                                          incomingMilli);
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
                            sensorRealTemp = readDoubleOrNan(sensorTempFile);
                            sensorTempFile.close();
                        }
                        else
                        {
                            // TODO(): Error message, with throttling, here
                            errorEncountered = true;
                        }
                    }
                    else if (sensorList[i][t->first].type == "file")
                    {
                        std::fstream sensorValueFile;
                        sensorValueFile.open(sensorList[i][t->first].path, std::ios::in);
                        if (sensorValueFile)
                        {
                            sensorRealTemp = readDoubleOrNan(sensorValueFile);
                            sensorValueFile.close();
                        }
                        else
                        {
                            // TODO(): Error message, with throttling, here
                            errorEncountered = true;
                        }
                    }

                    if (std::isfinite(sensorRealTemp))
                    {
                        // If configured unit not already degrees, convert to degrees
                        if (incomingMilli)
                        {
                            sensorRealTemp /= 1000.0;
                        }
                    }
                    else
                    {
                        // TODO(): Error message, with throttling, here
                        errorEncountered = true;
                    }
                }

                if (!(std::isfinite(sensorRealTemp)))
                {
                    if (emptyService)
                    {
                        continue;
                    }
                    errorEncountered = true;

                    // Sensor failure, unable to get reading
                    if (debugEnabled)
                    {
                        std::cerr << " ?";
                    }

                    continue;
                }

                // If sensor already in margin, then accept it as-is
                if (incomingMargin)
                {
                    // Remember this margin if it is the worst margin
                    if ( (!(std::isfinite(calibMarginTemp))) ||
                        sensorRealTemp < calibMarginTemp)
                    {
                        calibMarginTemp = sensorRealTemp;
                    }

                    // Sensor successful, already in margin format
                    if (debugEnabled)
                    {
                        std::cerr << " " << sensorRealTemp;
                    }

                    continue;
                }

                if (!(std::isfinite(sensorSpecTemp)))
                {
                    errorEncountered = true;

                    // Sensor failure, needed to know sensorSpecTemp to compute margin
                    if (debugEnabled)
                    {
                        std::cerr << " ?";
                    }

                    continue;
                }

                if (sensorList[i][t->first].parametersScalar == 0)
                {
                    sensorCalibTemp = static_cast<double>(skuConfig[i].setPoint) / 1000.0;
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
                    auto offsetVal = calOffsetValue(
                            skuConfig[i].setPoint,
                            sensorList[i][t->first].parametersScalar,
                            sensorSpecTemp,
                            sensorList[i][t->first].parametersTargetTemp,
                            sensorList[i][t->first].parametersTargetTempOffset);
                    sensorCalibTemp += offsetVal;
                    sensorCalibTemp *= sensorList[i][t->first].parametersScalar;
                }

                // Remember this margin if it is the worst margin
                if ( (!(std::isfinite(calibMarginTemp))) ||
                    sensorCalibTemp < calibMarginTemp)
                {
                    calibMarginTemp = sensorCalibTemp;
                }

                // Sensor successful, converted absolute to margin
                if (debugEnabled)
                {
                    std::cerr << " " << sensorCalibTemp;
                }
            }

            // If all sensors failed, provide NaN as output,
            // unlike previous versions, which provided 0 as output,
            // which was incorrect and misleading.
            // The latest version of phosphor-pid-control will accept NaN
            // correctly, and use it as an indication of a broken sensor,
            // to properly throw the thermal zone into failsafe mode.
            if (!(std::isfinite(calibMarginTemp)))
            {
                calibMarginTemp = std::numeric_limits<double>::quiet_NaN();
            }

            // If any sensors failed, propagate the failure, instead of
            // simply dropping the failed sensors from the computation,
            // if single-point-of-failure mode is active.
            if (spofEnabled && errorEncountered)
            {
                calibMarginTemp = std::numeric_limits<double>::quiet_NaN();
            }

            updateDbusMarginTemp(i, calibMarginTemp, skuConfig[i].targetPath);

            // Finish sensor line, indicate computed worst margin
            if (debugEnabled)
            {
                std::cerr << " => " << calibMarginTemp << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
