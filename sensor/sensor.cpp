#include <fstream>
#include <dirent.h>

#include "conf/conf.hpp"
#include "sensor/sensor.hpp"

struct dirent *drnt;

std::string getSensorHwmonNum(std::string partialPath)
{
    auto dir = opendir(partialPath.c_str());

    while ((dir != NULL) && ((drnt = readdir(dir)) != NULL))
    {
        std::string hwmonNum(drnt->d_name);
        if (hwmonNum.find("hwmon") != std::string::npos)
        {
            closedir(dir);
            return "/" + hwmonNum + "/";
        }
    }
    closedir(dir);

    return "";
}

std::string getSysPath(std::string path, std::string label)
{
    path += getSensorHwmonNum(path);

    if (!label.empty())
    {
        auto dir = opendir(path.c_str());

        while ((dir != NULL) && ((drnt = readdir(dir)) != NULL))
        {
            std::string addr(drnt->d_name);
            if (addr.find("label") != std::string::npos)
            {
                std::fstream labelFile;
                std::string labelFilePath = path;
                std::string labelFileContent;

                labelFilePath.append(addr);
                labelFile.open(labelFilePath, std::ios::in);
                if (labelFile)
                {
                    labelFile >> labelFileContent;
                }

                labelFile.close();

                if (labelFileContent == label)
                {
                    addr.replace(addr.end()-5, addr.end(), "input");
                    path += addr;
                }
            }
        }
        closedir(dir);

    }

    return path;
}
