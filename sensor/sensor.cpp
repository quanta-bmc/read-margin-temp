#include <dirent.h>

#include "conf.hpp"
#include "sensor/sensor.hpp"

struct dirent *drnt;

std::string getSensorDeviceAddr(std::string partialPath, std::string reg)
{
    DIR *dir;

    dir = opendir(partialPath.c_str());
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
    DIR *dir;

    dir = opendir(partialPath.c_str());
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
    std::string path;
    std::string upperPath;
    std::string channel = "channel-";

    upperPath = sensorConfig.upperPath;
    channel += std::to_string(sensorConfig.channel);

    path = upperPath;
    path += "/";
    path += channel;
    path += "/";
    path += getSensorDeviceAddr(path ,sensorConfig.reg);
    path += "/hwmon/";
    path += getSensorHwmonNum(path);
    path += "/";
    path += sensorConfig.lowerPath;

    return path;
}
