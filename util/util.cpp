#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <mutex>
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

// Enables logging of cache operations
bool debugCache = false;

// Sensor value cache is deemed stale after this many seconds without update
double valueCacheLifetimeSecs = -1.0;

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

std::map<std::string, std::map<std::string, std::string>> g_serviceCache;
std::mutex g_serviceCacheLock;

std::string getServiceCache(const std::string& keyA, const std::string& keyB)
{
    std::lock_guard<std::mutex> lock(g_serviceCacheLock);

    auto foundA = g_serviceCache.find(keyA);
    if (foundA == g_serviceCache.end())
    {
        return std::string();
    }

    auto foundB = foundA->second.find(keyB);
    if (foundB == foundA->second.end())
    {
        return std::string();
    }

    return foundB->second;
}

void setServiceCache(const std::string& keyA, const std::string& keyB, const std::string& value)
{
    std::lock_guard<std::mutex> lock(g_serviceCacheLock);

    // Do not allow empty strings to pollute cache
    if (keyA.empty())
    {
        return;
    }
    if (keyB.empty())
    {
        return;
    }
    if (value.empty())
    {
        return;
    }

    auto foundA = g_serviceCache.find(keyA);
    if (foundA == g_serviceCache.end())
    {
        g_serviceCache[keyA] = std::map<std::string, std::string>();
        foundA = g_serviceCache.find(keyA);
    }

    auto foundB = foundA->second.find(keyB);
    if (foundB == foundA->second.end())
    {
        foundA->second[keyB] = std::string();
        foundB = foundA->second.find(keyB);
    }

    foundB->second = value;

    if (debugCache)
    {
        std::cerr << "Service cache(" << keyA << ", " << keyB << ") updated: " << value << "\n";
    }
}

void clearServiceCache(const std::string& keyA, const std::string& keyB)
{
    std::lock_guard<std::mutex> lock(g_serviceCacheLock);

    auto foundA = g_serviceCache.find(keyA);
    if (foundA == g_serviceCache.end())
    {
        return;
    }

    auto foundB = foundA->second.find(keyB);
    if (foundB == foundA->second.end())
    {
        return;
    }

    foundA->second.erase(keyB);
    if (foundA->second.empty())
    {
        g_serviceCache.erase(keyA);
    }

    if (debugCache)
    {
        std::cerr << "Service cache(" << keyA << ", " << keyB << ") invalidated\n";
    }
}

// Returns text if sensor flagged any alarms, empty string if sensor good
std::string checkSensorAlarmProperties(sdbusplus::bus::bus& bus, const std::string& service, const std::string& sensorDbusPath)
{
    // If sensor functional property is false, set NaN to dbus and return NaN.
    if (!dbus::SDBusPlus::checkFunctionalProperty(bus,
                                                  service,
                                                  sensorDbusPath))
    {
        return "NonFunctional";
    }

    if (dbus::SDBusPlus::checkWarningProperty(bus,
                                              service,
                                              sensorDbusPath,
                                              "WarningAlarmHigh"))
    {
        return "WarningAlarmHigh";
    }

    if (dbus::SDBusPlus::checkWarningProperty(bus,
                                              service,
                                              sensorDbusPath,
                                              "WarningAlarmLow"))
    {
        return "WarningAlarmLow";
    }

    if (dbus::SDBusPlus::checkCriticalProperty(bus,
                                               service,
                                               sensorDbusPath,
                                               "CriticalAlarmHigh"))
    {
        return "CriticalAlarmHigh";
    }

    if (dbus::SDBusPlus::checkCriticalProperty(bus,
                                               service,
                                               sensorDbusPath,
                                               "CriticalAlarmLow"))
    {
        return "CriticalAlarmLow";
    }

    return "";
}

class ValueCacheEntry;
std::map<std::string, std::unique_ptr<ValueCacheEntry>> g_valueCache;
std::mutex g_valueCacheLock;

std::string matchString(const std::string& objectPath)
{
    // arg0namespace intentionally not specified, will match all interfaces
    std::string matcher = "type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path='";
    matcher += objectPath;
    matcher += "'";
    return matcher;
}

void triggerValueCache(sdbusplus::message::message& msg, const std::string& objectPath);
void matchCallback(sdbusplus::message::message& msg, const std::string& objectPath)
{
    if (debugCache)
    {
        std::cerr << "Sensor change notification triggered: " << objectPath << "\n";
    }
    triggerValueCache(msg, objectPath);
}

class ValueCacheEntry
{
private:
    sdbusplus::bus::match::match matcher;
    std::string sensor;

public:
    std::chrono::time_point<std::chrono::steady_clock> when;
    double value;
    double received;
    bool valid;
    bool triggered;
    bool alarmed;

    ValueCacheEntry(const std::string& objectPath, sdbusplus::bus::bus& bus)
        : matcher(bus, matchString(objectPath), [o = objectPath](sdbusplus::message::message& m){ matchCallback(m, o); })
        , sensor(objectPath)
        , when(std::chrono::steady_clock::now())
        , value(std::numeric_limits<double>::quiet_NaN())
        , received(std::numeric_limits<double>::quiet_NaN())
        , valid(false)
        , triggered(false)
        , alarmed(false)
    {
    }
};

double getValueCache(const std::string& objectPath, sdbusplus::bus::bus& bus, bool* outAlarm)
{
    std::lock_guard<std::mutex> lock(g_valueCacheLock);

    auto now = std::chrono::steady_clock::now();

    double nan = std::numeric_limits<double>::quiet_NaN();

    // Do not allow empty strings to pollute cache
    if (objectPath.empty())
    {
        return nan;
    }

    auto found = g_valueCache.find(objectPath);
    if (found == g_valueCache.end())
    {
        g_valueCache[objectPath] = std::make_unique<ValueCacheEntry>(objectPath, bus);
        found = g_valueCache.find(objectPath);
    }

    if (found->second->triggered)
    {
        // Consume the trigger notification, while we have the lock
        found->second->triggered = false;

        // If alarm indicated, disregard cache until alarm condition clears
        if (found->second->alarmed)
        {
            if (debugCache)
            {
                std::cerr << "Alarm notification received, invalidating cache: " << objectPath << "\n";
            }
            found->second->valid = false;
            if (outAlarm)
            {
                *outAlarm = true;
            }
            return nan;
        }

        // If good value already received, no need to fetch manually
        if (std::isfinite(found->second->received))
        {
            if (debugCache)
            {
                std::cerr << "Sensor notification received, updating value: " << found->second->value << " from " << objectPath << "\n";
            }
            found->second->when = now;
            found->second->value = found->second->received;
            found->second->valid = true;
            return found->second->value;
        }

        // If no value received, invalidate cache, to fetch manually
        if (debugCache)
        {
            std::cerr << "Sensor notification received, refreshing cache: " << objectPath << "\n";
        }
        found->second->valid = false;
        return nan;
    }

    if (found->second->alarmed)
    {
        if (outAlarm)
        {
            *outAlarm = true;
        }
        return nan;
    }

    if (!(found->second->valid))
    {
        return nan;
    }

    // Timeout feature is optional, disabled if negative
    if (valueCacheLifetimeSecs > 0.0)
    {
        std::chrono::steady_clock::duration age = now - found->second->when;
        if (age > std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(valueCacheLifetimeSecs)))
        {
            if (debugCache)
            {
                std::cerr << "Sensor value has remained unchanged, refreshing cache: " << objectPath << "\n";
            }
            found->second->valid = false;
            return nan;
        }
    }

    return found->second->value;
}

void setValueCacheValue(const std::string& objectPath, double value)
{
    std::lock_guard<std::mutex> lock(g_valueCacheLock);

    auto now = std::chrono::steady_clock::now();

    // Do not allow NaN to pollute value cache
    if (!(std::isfinite(value)))
    {
        return;
    }

    auto found = g_valueCache.find(objectPath);
    if (found == g_valueCache.end())
    {
        return;
    }

    // Intentionally do not touch member "triggered", or you will race
    found->second->when = now;
    found->second->value = value;
    found->second->valid = true;

    if (debugCache)
    {
        std::cerr << "Sensor reading of " << value << " from " << objectPath << "\n";
    }
}

void clearValueCache(const std::string& objectPath)
{
    std::lock_guard<std::mutex> lock(g_valueCacheLock);

    auto found = g_valueCache.find(objectPath);
    if (found == g_valueCache.end())
    {
        return;
    }

    // Simply mark as false, no need to erase
    found->second->valid = false;
}

void setValueCacheAlarm(const std::string& objectPath, bool isAlarm)
{
    std::lock_guard<std::mutex> lock(g_valueCacheLock);

    auto found = g_valueCache.find(objectPath);
    if (found == g_valueCache.end())
    {
        return;
    }

    found->second->alarmed = isAlarm;
}

// Avoid having to fetch value as a separate step later,
// if new value can be parsed out of incoming message.
double parseMessage(sdbusplus::message::message& msg, const std::string& source, bool *outAlarm)
{
    double nan = std::numeric_limits<double>::quiet_NaN();

    std::string sig = msg.get_signature();
    if (sig != "sa{sv}as")
    {
        std::cerr << "Message received, but unrecognized signature: " << sig << " from " << source << "\n";
        return nan;
    }

    std::string interface;
    std::map<std::string, std::variant<double, bool>> content;

    try
    {
        msg.read(interface, content);
    }
    catch(const sdbusplus::exception::exception& e)
    {
        std::cerr << "Message received, but unparseable: " << e.name() << "(" << e.description() << ") from " << source << "\n";
        return nan;
    }

    if (interface != VALUEINTERFACE)
    {
        // If not a value, it might be an alarm interface instead
        bool isAlarmInterface = false;
        if ((interface == FUNCTIONALINTERFACE)
         || (interface == WARNINGINTERFACE)
         || (interface == CRITICALINTERFACE))
        {
            isAlarmInterface = true;
        }

        if (!isAlarmInterface)
        {
            std::cerr << "Message received, but unrecognized interface: " << interface << " from " << source << "\n";
            return nan;
        }

        // See if a known alarm
        bool badIndication = true;
        auto found = content.find("Functional");
        if (found != content.end())
        {
            // Functional is inverted logic, true indicates good condition
            badIndication = false;
        }

        // All other alarms are consistent, false indicates good condition
        if (found == content.end())
        {
            found = content.find("WarningAlarmHigh");
        }
        if (found == content.end())
        {
            found = content.find("WarningAlarmLow");
        }
        if (found == content.end())
        {
            found = content.find("CriticalAlarmHigh");
        }
        if (found == content.end())
        {
            found = content.find("CriticalAlarmLow");
        }
        if (found == content.end())
        {
            std::cerr << "Alarm received, but unrecognized content: " << interface << " from " << source << "\n";
            return nan;
        }

        const bool* bptr = std::get_if<bool>(&(found->second));
        if (!bptr)
        {
            std::cerr << "Alarm received, but unparseable content: " << source << "\n";
            return nan;
        }

        bool alarmIndication = *bptr;
        if (alarmIndication == badIndication)
        {
            // Good reception of alarm signal, indication is of bad condition
            std::cerr << "Alarm received, bad condition indicated: " << source << "\n";
            if (outAlarm)
            {
                *outAlarm = true;
            }
            return nan;
        }

        // Good reception of alarm signal, indication is of good condition
        if (debugCache)
        {
            std::cerr << "Alarm received, good condition indicated: " << source << "\n";
        }
        return nan;
    }

    auto found = content.find("Value");
    if (found == content.end())
    {
        std::cerr << "Message received, but no value: " << source << "\n";
        return nan;
    }

    const double* dptr = std::get_if<double>(&(found->second));
    if (!dptr)
    {
        std::cerr << "Message received, but unparseable value: " << source << "\n";
        return nan;
    }

    double reading = *dptr;
    if (!(std::isfinite(reading)))
    {
        std::cerr << "Message received, but invalid value: " << source << "\n";
        return nan;
    }

    if (debugCache)
    {
        std::cerr << "Received value update: " << reading << " from " << source << "\n";
    }
    return reading;
}

void triggerValueCache(sdbusplus::message::message& msg, const std::string& objectPath)
{
    std::lock_guard<std::mutex> lock(g_valueCacheLock);

    auto found = g_valueCache.find(objectPath);
    if (found == g_valueCache.end())
    {
        if (debugCache)
        {
            std::cerr << "Message received, but unrecognized object: " << objectPath << "\n";
        }
        return;
    }

    bool isAlarmed = false;
    double value = parseMessage(msg, objectPath, &isAlarmed);

    // Mark object as triggered, so next cache get will pick up our changes
    found->second->triggered = true;
    found->second->received = value;
    if (isAlarmed)
    {
        found->second->alarmed = true;
    }
}

double getSensorDbusTemp(std::string sensorDbusPath, bool unitMilli)
{
    sdbusplus::bus::bus& bus = *g_default_bus;

    bool isAlarmed = false;
    double value = getValueCache(sensorDbusPath, bus, &isAlarmed);

    // If value cache good, no need to continue with expensive D-Bus calls
    if (std::isfinite(value))
    {
        return value;
    }

    std::string service = getService(sensorDbusPath, VALUEINTERFACE);

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

            // Invalidate service name cache for both lookups upon error
            clearServiceCache(nvmeInventoryPath, PRESENTINTERFACE);
            clearServiceCache(sensorDbusPath, VALUEINTERFACE);
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

    std::string alarmText = checkSensorAlarmProperties(bus, service, sensorDbusPath);

    // Invalidate service cache upon indication of anomaly
    if (!(alarmText.empty()))
    {
        if (debugCache)
        {
            std::cerr << "Sensor anomaly " << alarmText << " indicated by " << sensorDbusPath << "\n";
        }

        // Invalidate value cache, so anomaly persists until alarm clears
        // This will slow performance, but alarm is not a normal operating condition
        clearValueCache(sensorDbusPath);

        clearServiceCache(sensorDbusPath, VALUEINTERFACE);
        return value;
    }

    // No anomaly detected, can clear previous alarm
    if (isAlarmed)
    {
        if (debugCache)
        {
            std::cerr << "Sensor anomaly cleared by " << sensorDbusPath << "\n";
        }
        setValueCacheAlarm(sensorDbusPath, false);
    }

    value = dbus::SDBusPlus::getValueProperty(bus,
                                              service,
                                              sensorDbusPath,
                                              unitMilli);

    if (std::isfinite(value))
    {
        setValueCacheValue(sensorDbusPath, value);
    }
    else
    {
        // Invalidate service cache, value cache already is invalid
        clearServiceCache(sensorDbusPath, VALUEINTERFACE);
    }

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

std::string getService(const std::string& dbusPath, const std::string& interfacePath)
{
    std::string cacheHit = getServiceCache(dbusPath, interfacePath);
    if (!(cacheHit.empty()))
    {
        return cacheHit;
    }

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

    std::string cacheUpdate = response.begin()->first;
    setServiceCache(dbusPath, interfacePath, cacheUpdate);
    return cacheUpdate;
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
    if (!(dbus::SDBusPlus::setValueProperty(bus,
                                      service,
                                      targetPath,
                                      marginTemp,
                                      false)))
    {
        // Invalidate service cache if the setting of the value failed
        clearServiceCache(targetPath, VALUEINTERFACE);
    }
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
        // Handle all incoming message notifications
        bool handleMore = true;
        while(handleMore)
        {
            // Keep handling more messages until this returns false
            handleMore = g_default_bus->process_discard();
        }

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
