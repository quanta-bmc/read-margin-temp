#include "conf/conf.hpp"
#include "sensor/sensor.hpp"
#include <fstream>

void conf::sensorComponents::determineUnit()
{
    // Numbers from D-Bus or filesystem need to know their units
    if (unit.find("milli") != std::string::npos)
    {
        _incomingMilli = true;
    }
    // Also need to know if units are absolute or margin
    if (unit.find("margin") != std::string::npos)
    {
        _incomingMargin = true;
    }
}

// As per documentation, specTemp unit, in config or filesystem, is always millidegrees
// However, this function set Spectemp in degrees
void conf::sensorComponents::setSpecTemp()
{
    // As per documentation, -1 indicates "value comes from outside the config"
    if (parametersMaxTemp != -1)
    {
        // The value is within the config itself, use it as-is
        _sensorSpecTemp = static_cast<double>(parametersMaxTemp);

        // Convert millidegrees to degrees
        _sensorSpecTemp /= 1000.0;
        return;
    }

    std::fstream sensorSpecFile;

    if (parametersType == "sys")
    {
        std::string path;

        path = getSysPath(parametersPath, parametersSysLabel);
        sensorSpecFile.open(path, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> _sensorSpecTemp;
            sensorSpecFile.close();
        }
    }
    else if (parametersType == "file")
    {
        sensorSpecFile.open(parametersPath, std::ios::in);
        if (sensorSpecFile)
        {
            sensorSpecFile >> _sensorSpecTemp;
            sensorSpecFile.close();
        }
    }

    if (std::isnan(_sensorSpecTemp))
    {
        // std::cerr << "Sensor MaxTemp reading not available: " << config.parametersPath << std::endl;
    }
    else
    {
        // Convert millidegrees to degrees
        _sensorSpecTemp /= 1000.0;
    }

    return;
}

void conf::sensorComponents::setSetPointVal(int val)
{
    // Convert millimargin to margin
    _setPoint = static_cast<double>(val) / 1000.0;
}

bool conf::sensorComponents::getUnitMilli()
{
    return _incomingMilli;
}

bool conf::sensorComponents::getUnitMargin()
{
    return _incomingMargin;
}

// This function already returns degrees
double conf::sensorComponents::getSpecTemp()
{
    return _sensorSpecTemp;
}

// This function already returns degrees
double conf::sensorComponents::getSetPointVal()
{
    return _setPoint;
}

// Avoid losing precision by doing calculations as double
double conf::sensorComponents::getOffsetVal()
{
    // All integers are in millidegrees, convert to degrees
    double targetOffset = static_cast<double>(parametersTargetTempOffset);
    targetOffset /= 1000.0;

    double offsetvalue = 0.0;
    offsetvalue = _setPoint / parametersScalar;

    // If targetTemp not specified, use maxTemp instead
    double targetTemp;
    if (parametersTargetTemp == -1)
    {
        targetTemp = _sensorSpecTemp;
    }
    else
    {
        targetTemp = static_cast<double>(parametersTargetTemp);
        targetTemp /= 1000.0;
    }

    offsetvalue -= _sensorSpecTemp - ( targetTemp + targetOffset );
    return offsetvalue;
}
