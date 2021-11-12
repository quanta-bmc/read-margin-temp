#pragma once

#include <iostream>
#include <string>
#include <map>
#include <variant>
#include <cmath>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

using PropertyMap =
    std::map<std::string, std::variant<int64_t, double, std::string, bool>>;

constexpr auto DBUSPROPERTYINTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto VALUEINTERFACE = "xyz.openbmc_project.Sensor.Value";
constexpr auto FUNCTIONALINTERFACE = "xyz.openbmc_project.State.Decorator.OperationalStatus";
constexpr auto WARNINGINTERFACE = "xyz.openbmc_project.Sensor.Threshold.Warning";
constexpr auto CRITICALINTERFACE = "xyz.openbmc_project.Sensor.Threshold.Critical";
constexpr auto PRESENTINTERFACE = "xyz.openbmc_project.Inventory.Item";

namespace dbus
{
class SDBusPlus
{
    public:
        /**
         * Set value to dbus.
         *
         * @param[in] bus - bus.
         * @param[in] busName - bus name.
         * @param[in] objPath - object path.
         * @param[in] value - value to be set, in degrees, regardless of unitMilli.
         * @param[in] unitMilli - true to set millidegrees, false to set degrees.
         * @return True if successful, false if error
         */
        static bool setValueProperty(sdbusplus::bus::bus& bus,
                                     const std::string& busName,
                                     const std::string& objPath,
                                     const double& value,
                                     bool unitMilli)
        {
            try
            {
                auto methodCall = bus.new_method_call(
                    busName.c_str(), objPath.c_str(), DBUSPROPERTYINTERFACE, "Set");

                methodCall.append(VALUEINTERFACE);
                methodCall.append("Value");

                // Using millidegrees on D-Bus seems to need int64_t not double
                if (unitMilli)
                {
                    // Convert degrees to millidegrees
                    auto intValue = static_cast<int64_t>(value * 1000.0);

                    // Send as int64_t not double
                    methodCall.append(std::variant<int64_t>(intValue));
                }
                else
                {
                    // Send as double
                    methodCall.append(std::variant<double>(value));
                }

                bus.call_noreply(methodCall);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Set dbus properties fail. " << e.what() << std::endl;
                return false;
            }

            return true;
        }

        /**
         * Get value from dbus.
         *
         * @param[in] bus - bus.
         * @param[in] service - dbus service name.
         * @param[in] path - dbus path.
         * @param[in] unitMilli - true to get millidegrees, false to get degrees.
         * @return Value from dbus, in degrees, regardless of unitMilli.
         */
        static double getValueProperty(sdbusplus::bus::bus& bus,
                                       const std::string& service,
                                       const std::string& path,
                                       bool unitMilli)
        {
            PropertyMap propMap;
            auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                DBUSPROPERTYINTERFACE, "GetAll");
            pimMsg.append(VALUEINTERFACE);

            double value = std::numeric_limits<double>::quiet_NaN();

            try
            {
                auto valueResponseMsg = bus.call(pimMsg);
                valueResponseMsg.read(propMap);
            }
            catch (const std::exception& e)
            {
                // std::cerr << "Get dbus properties fail. " << e.what() << std::endl;
                return value;
            }

            // Using millidegrees on D-Bus seems to need int64_t not double
            if (unitMilli)
            {
                // Receive as int64_t not double
                auto intValue = std::get<int64_t>(propMap["Value"]);

                // Sensors in integer use -1 to indicate error
                if (intValue != -1)
                {
                    // Convert millidegrees to degrees
                    // Avoid loss of precision by first casting to double
                    value = static_cast<double>(intValue);
                    value /= 1000.0;
                }
            }
            else
            {
                // Receive as double
                value = std::get<double>(propMap["Value"]);
            }

            // Sensors in floating-point use nan to indicate error
            if (!(std::isfinite(value)))
            {
                // std::cerr << "Sensor Value reading not available: " << path << std::endl;
            }
            return value;
        }
        /**
         * Check dbus sensor WarningAlarm property.
         *
         * @param[in] bus - bus.
         * @param[in] service - dbus service name.
         * @param[in] path - dbus path.
         * @param[in] property - dbus property name
         * @return WarningAlarm property from dbus
         */
        static bool checkWarningProperty(sdbusplus::bus::bus& bus,
                                         const std::string& service,
                                         const std::string& path,
                                         const std::string& property)
        {
            PropertyMap propMap;
            auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                DBUSPROPERTYINTERFACE, "GetAll");
            pimMsg.append(WARNINGINTERFACE);

            bool warningAlarm = false;

            try
            {
                auto warningHighResponseMsg = bus.call(pimMsg);
                warningHighResponseMsg.read(propMap);
                warningAlarm = std::get<bool>(propMap[property]);
            }
            catch (const std::exception& e)
            {
                // std::cerr << "Get WarningAlarm property failed." << std::endl;
            }

            // std::cerr << property << " : " << warningAlarm << std::endl;
            return warningAlarm;
        }
        /**
         * Check dbus sensor CriticalAlarm property.
         *
         * @param[in] bus - bus.
         * @param[in] service - dbus service name.
         * @param[in] path - dbus path.
         * @param[in] property - dbus property name
         * @return CriticalAlarm property from dbus
         */
        static bool checkCriticalProperty(sdbusplus::bus::bus& bus,
                                          const std::string& service,
                                          const std::string& path,
                                          const std::string& property)
        {
            PropertyMap propMap;
            auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                DBUSPROPERTYINTERFACE, "GetAll");
            pimMsg.append(CRITICALINTERFACE);

            bool criticalAlarm = false;

            try
            {
                auto criticalHighResponseMsg = bus.call(pimMsg);
                criticalHighResponseMsg.read(propMap);
                criticalAlarm = std::get<bool>(propMap[property]);
            }
            catch (const std::exception& e)
            {
                // std::cerr << "Get CriticalAlarm property failed." << std::endl;
            }

            // std::cerr << property << " : " << criticalAlarm << std::endl;
            return criticalAlarm;
        }
        /**
         * Get dbus sensor functional property.
         *
         * @param[in] bus - bus.
         * @param[in] service - dbus service name.
         * @param[in] path - dbus path.
         * @return Functional property from dbus
         */
        static bool checkFunctionalProperty(sdbusplus::bus::bus& bus,
                                            const std::string& service,
                                            const std::string& path)
        {
            PropertyMap propMap;
            auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                DBUSPROPERTYINTERFACE, "GetAll");
            pimMsg.append(FUNCTIONALINTERFACE);

            bool functional = true;

            try
            {
                auto functionalResponseMsg = bus.call(pimMsg);
                functionalResponseMsg.read(propMap);
                functional = std::get<bool>(propMap["Functional"]);
            }
            catch (const std::exception& e)
            {
                // std::cerr << "Get Functional property failed." << std::endl;
            }

            // std::cerr << "Functional : " << functional << std::endl;
            return functional;
        }
        /**
         * Get nvme sensor Present property.
         *
         * @param[in] bus - bus.
         * @param[in] service - dbus service name.
         * @param[in] path - dbus path.
         * @return Present property from dbus
         */
        static bool checkNvmePresentProperty(sdbusplus::bus::bus& bus,
                                             const std::string& service,
                                             const std::string& path)
        {
            PropertyMap propMap;
            auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                DBUSPROPERTYINTERFACE, "GetAll");
            pimMsg.append(PRESENTINTERFACE);
            bool present = false;
            try
            {
                auto presentResponseMsg = bus.call(pimMsg);
                presentResponseMsg.read(propMap);
                present = std::get<bool>(propMap["Present"]);
            }
            catch (const std::exception& e)
            {
                // std::cerr << "Get Present property failed." << std::endl;
            }
            // std::cerr << property << " : " << present << std::endl;
            return present;
        }
};
}
