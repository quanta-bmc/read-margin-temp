#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <variant>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using propertyMap = 
    std::map<std::string, std::variant<int64_t, double, std::string, bool>>;

constexpr auto marginTempPath = "/xyz/openbmc_project/extsensors/margin";
constexpr auto dbusPropertyIface = "org.freedesktop.DBus.Properties";

struct variantToDoubleVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        return static_cast<double>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        throw ;
    }
};

namespace dbus
{
class SDBusPlus
{
public:
    template <typename T>
    static auto
        setProperty(sdbusplus::bus::bus& bus, const std::string& busName,
            const std::string& objPath, const std::string& interface,
            const std::string& property, const T& value)
    {
        sdbusplus::message::variant<T> data = value;

        try
        {
            auto methodCall = bus.new_method_call(
                busName.c_str(), objPath.c_str(), dbusPropertyIface, "Set");

            methodCall.append(interface.c_str());
            methodCall.append(property);
            methodCall.append(data);

            auto reply = bus.call(methodCall);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Set properties fail. ERROR = " << e.what()
                << std::endl;
            std::cerr << "Object path = " << objPath << std::endl;
            return;
        }
    }

    static double getProperty(sdbusplus::bus::bus& bus,
        const std::string& service, const std::string& path,
        const std::string& interface, const std::string& property)
    {
        auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                                        dbusPropertyIface, "GetAll");

        pimMsg.append(interface.c_str());

        propertyMap propMap;

        try
        {
            auto valueResponseMsg = bus.call(pimMsg);
            valueResponseMsg.read(propMap);
        }
        catch (const sdbusplus::exception::SdBusError& ex)
        {
            return -1;
        }

        return std::visit(variantToDoubleVisitor(), propMap[property]);
    }

};
}
