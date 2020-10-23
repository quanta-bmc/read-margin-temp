#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace conf
{
class sensorComponents
{
public:
    std::string name;
    std::string unit;
    std::string type;
    std::string path;

    /** parameters info **/
    std::string parametersType;
    int parametersMaxTemp;
    std::string parametersPath;
    std::string parametersSysLabel;
    int parametersTargetTemp;
    int parametersTargetTempOffset = 0;
    double parametersScalar;

    void determineUnit();
    void setSpecTemp();
    void setSetPointVal(int val);
    bool getUnitMilli();
    bool getUnitMargin();
    double getSpecTemp();
    double getSetPointVal();
    double getOffsetVal();

private:
    double _setPoint;
    bool _incomingMilli = false;
    bool _incomingMargin = false;
    double _sensorRealTemp = std::numeric_limits<double>::quiet_NaN();
    double _sensorSpecTemp = std::numeric_limits<double>::quiet_NaN();
};

struct zoneConfig
{
    int id;
    int setpoint;
    std::string targetPath;
    std::vector<std::string> components;
};

using skuConfig = std::map<int, conf::zoneConfig>;
}
