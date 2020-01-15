#include <fstream>
#include <string>
#include <chrono>
#include <thread>

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

void updateMarginTempLoop()
{
        /*** update fleeting0 temp ***/
    std::fstream sensorTempFile;
    std::string tmp;
    int sensorRealTemp = 0;
    int sensorSpecTemp = 0;
    int sensorMarginTemp = 0;
    int marginTemp = -1;
    std::pair<std::string, int> sensorList[NUMBER_OF_SENSOR];

    sensorList[0] = std::make_pair(getTempI2coolPath(0), TEMP_I2COOL_0_SPEC);
    sensorList[1] = std::make_pair(getTempI2coolPath(1), TEMP_I2COOL_1_SPEC);
    sensorList[2] = std::make_pair(getTempI2coolPath(2), TEMP_I2COOL_2_SPEC);
    sensorList[3] = std::make_pair(getTempI2coolPath(3), TEMP_I2COOL_3_SPEC);

    while (true)
    {
        for (int i = 0; i < NUMBER_OF_SENSOR; i++)
        {
            sensorTempFile.open(sensorList[i].first, std::ios::in);
            sensorSpecTemp = sensorList[i].second;
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
