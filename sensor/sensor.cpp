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

std::string getSysPath(std::string path,
    std::string input, int channelNum, std::string reg)
{
    std::string channel = "channel-";

    if (channelNum != -1)
    {
        channel += std::to_string(channelNum);
        path += "/";
        path += channel;
    }
    path += "/";
    if (!reg.empty())
    {
        path += getSensorDeviceAddr(path ,reg);
        path += "/hwmon/";
        path += getSensorHwmonNum(path);
    }
    else
    {
        path += getSensorHwmonNum(path);
    }

    path += "/";
    path += input;

    return path;
}
