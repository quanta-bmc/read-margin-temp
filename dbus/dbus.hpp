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

constexpr auto dbusPropertyInterface = "org.freedesktop.DBus.Properties";
constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
constexpr auto valueProperty = "Value";

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
    static auto
        setValueProperty(sdbusplus::bus::bus& bus, const std::string& busName,
            const std::string& objPath, const int64_t& value)
    {
        sdbusplus::message::variant<int64_t> data = value;

        try
        {
            auto methodCall = bus.new_method_call(
                busName.c_str(), objPath.c_str(), dbusPropertyInterface, "Set");

            methodCall.append(valueInterface);
            methodCall.append(valueProperty);
            methodCall.append(data);

            auto reply = bus.call(methodCall);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Set properties fail. ERROR = " << e.what()
                << std::endl;
            return;
        }
    }

    static double getValueProperty(sdbusplus::bus::bus& bus,
        const std::string& service, const std::string& path)
    {
        propertyMap propMap;
        auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
            dbusPropertyInterface, "GetAll");
        pimMsg.append(valueInterface);

        try
        {
            auto valueResponseMsg = bus.call(pimMsg);
            valueResponseMsg.read(propMap);
        }
        catch (const std::exception& e)
        {
            return -1;
        }

        return std::visit(variantToDoubleVisitor(), propMap[valueProperty]);
    }

};
}
