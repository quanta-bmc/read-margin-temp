#include <dirent.h>

#include "sensor/sensor.hpp"

struct dirent *drnt;

std::string getFanDeviceAddr(int num)
{
    std::string path = FAN_PATH;
    std::string channel = "/channel-2/";
    DIR *dir;

    if (num != 0)
    {
        channel = "/channel-3/";
    }
    path += channel;
    dir = opendir(path.c_str());
    while ((drnt = readdir(dir)) != NULL) 
    {
        std::string name(drnt->d_name);
        if (name.find("002c") != std::string::npos)
        {
            closedir(dir);
            return name;
        }
    }
    closedir(dir);

    return "";
}

std::string getFan6To11Path()
{
    std::string fanPath = FAN_PATH;
    std::string fanDeviceAddr = getFanDeviceAddr(1);
    DIR *dir;

    if (fanDeviceAddr.empty())
    {
        return "";
    }
    fanPath += "/channel-3/";
    fanPath += fanDeviceAddr;
    fanPath += "/hwmon/";
    dir = opendir(fanPath.c_str());
    while ((drnt = readdir(dir)) != NULL) 
    {
        std::string name(drnt->d_name);
        if (name.find("hwmon") != std::string::npos)
        {
            fanPath += name;
            closedir(dir);
            return fanPath;
        }
    }
    closedir(dir);

    return "";
}

std::string getTempI2coolPath(int num)
{
    std::string tempI2coolPath = I2COOL_PATH;
    std::string cmd = "";
    std::string channel = "";
    // DIR *dir;

    if (num == 0)
    {
        channel = "/channel-3/";
    }
    else if (num == 1)
    {
        channel = "/channel-4/";
    }
    else if (num == 2)
    {
        channel = "/channel-5";
    }
    else
    {
        channel = "/channel-6";
    }

    // dir = opendir(tempI2coolPath.c_str());

    // cmd = "ls -la ";
    // cmd += I2COOL_PATH;
    // cmd += channel;
    // cmd += " | grep 005c | awk '{print $9}'";
    // cmd = executeCmd(cmd);
    // cmd.erase(cmd.end()-1, cmd.end());
    // tempI2coolPath += channel;
    // tempI2coolPath += cmd;
    // cmd = "ls -la ";
    // cmd += tempI2coolPath;
    // cmd += "/hwmon/ | grep hwmon | awk '{print $9}'";
    // cmd = executeCmd(cmd);
    // cmd.erase(cmd.end()-1, cmd.end());
    // tempI2coolPath += "/hwmon/";
    // tempI2coolPath += cmd;
    // tempI2coolPath += "/temp1_input";

    return tempI2coolPath;
}
