

#include <sdbusplus/server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/Sensor/Value/server.hpp>

using ValueInterface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;
using MarginTempIface = 
    sdbusplus::server::object::object<ValueInterface>;

#define MARGIN_TEMP_PATH "/xyz/openbmc_project/extsensors/margin"

class MarginTemp : public MarginTempIface
{
public:
    MarginTemp() = delete;
    MarginTemp(const MarginTemp&) = delete;
    MarginTemp& operator=(const MarginTemp&) = delete;
    MarginTemp(MarginTemp&&) = delete;
    MarginTemp& operator=(MarginTemp&&) = delete;
    virtual ~MarginTemp() = default;

    /** @brief Constructs MarginTemp
     *
     * @param[in] bus     - Handle to system dbus
     * @param[in] objPath - The Dbus path of MarginTemp
     */
    MarginTemp(sdbusplus::bus::bus& bus, const char* objPath) :
        MarginTempIface(bus, objPath), bus(bus)
    {
    }

    void setSensorValueToDbus(int value);

private:
    sdbusplus::bus::bus& bus;
};
