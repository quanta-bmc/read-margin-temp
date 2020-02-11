#include <dirent.h>

#include "conf.hpp"
#include "sensor/sensor.hpp"

struct dirent *drnt;

std::string getSensorDeviceAddr(std::string partialPath, std::string reg)
{
    auto dir = opendir(partialPath.c_str());

    while ((drnt = readdir(dir)) != NULL)
    {
        std::string addr(drnt->d_name);
        if (addr.find(reg.c_str()) != std::string::npos)
        {
            closedir(dir);
            return addr;
        }
    }
    closedir(dir);

    return "";
}

std::string getSensorHwmonNum(std::string partialPath)
{
    auto dir = opendir(partialPath.c_str());

    while ((drnt = readdir(dir)) != NULL) 
    {
        std::string hwmonNum(drnt->d_name);
        if (hwmonNum.find("hwmon") != std::string::npos)
        {
            closedir(dir);
            return hwmonNum;
        }
    }
    closedir(dir);

    return "";
}

std::string getSensorPath(struct conf::sensorConfig sensorConfig)
{
    if (sensorConfig.pathType == "dbus")
    {
        return sensorConfig.dbusPath;
    }

    std::string path = sensorConfig.sysPath;
    std::string channel = "channel-";

    if (sensorConfig.sysChannel != -1)
    {
        channel += std::to_string(sensorConfig.sysChannel);
        path += "/";
        path += channel;
    }
    path += "/";
    if (!sensorConfig.sysReg.empty())
    {
        path += getSensorDeviceAddr(path ,sensorConfig.sysReg);
        path += "/hwmon/";
        path += getSensorHwmonNum(path);
    }
    else
    {
        path += getSensorHwmonNum(path);
    }

    path += "/";
    path += sensorConfig.sysInput;

    return path;
}
