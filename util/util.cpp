#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <map>
#include <vector>

#include "conf.hpp"
#include "util/util.hpp"
#include "sensor/sensor.hpp"

int getSkuNum()
{
    std::string fanPath;
    std::fstream fanPwmFile;
    std::fstream fanRpmFile;
    bool fan6To11Control[6] = {false, false, false, false, false, false};
    int skuNum = 1;
    int fanRpm = 0;

    for (int i = 0; i < 6; i++)
    {
        fanPath = getFan6To11Path();
        if (fanPath.empty())
        {
            return -1;
        }
        fanPath += "/pwm";
        fanPath += std::to_string(i+1);
        fanPwmFile.open(fanPath, std::ios::out);
        fanPwmFile << 255;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        fanPwmFile.close();
        fanPath = getFan6To11Path();
        if (fanPath.empty())
        {
            return -1;
        }
        fanPath += "/fan";
        fanPath += std::to_string(i+1);
        fanPath += "_input";
        fanRpmFile.open(fanPath, std::ios::in);
        fanRpmFile >> fanRpm;
        fanRpmFile.close();
        if (fanRpm > 2000)
        {
            fan6To11Control[i] = true;
        }
    }

    if (fan6To11Control[0])
    {
        if (fan6To11Control[2])
        {
            skuNum = 2;
        }
        else
        {
            skuNum = 3;
        }
    }

    return skuNum;
}

void updateMarginTempLoop(
    std::map<int, std::vector<std::string>> skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig)
{
    std::fstream sensorTempFile;
    std::string tmp;
    int numOfZones = skuConfig.size();
    int sensorRealTemp;
    int sensorSpecTemp;
    int sensorMarginTemp;
    int marginTemp;
    std::map<std::string, struct conf::sensorConfig> sensorList[numOfZones];

    for (int i = 0; i < numOfZones; i++)
    {
        for (auto t = skuConfig[i].begin(); t != skuConfig[i].end(); t++)
        {
            sensorList[i][*t] = sensorConfig[*t];
        }
    }

    while (true)
    {
        for (int i = 0; i < numOfZones; i++)
        {
            marginTemp = -1;
            for (auto t = sensorList[i].begin(); t != sensorList[i].end(); t++)
            {
                sensorRealTemp = 0;
                sensorSpecTemp = sensorList[i][t->first].spec;
                sensorTempFile.open(getSensorPath(t->second), std::ios::in);
                if (sensorTempFile)
                {
                    sensorTempFile >> sensorRealTemp;
                }
                sensorTempFile.close();
                sensorMarginTemp = (sensorSpecTemp - sensorRealTemp) / 1000;

                if (marginTemp == -1 || sensorMarginTemp < marginTemp)
                {
                    marginTemp = sensorMarginTemp;
                }
            }
            tmp = dbusSetPropertyCommand + std::to_string(sensorMarginTemp);
            system(tmp.c_str());

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
