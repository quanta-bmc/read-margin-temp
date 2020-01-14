#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <utility>

#define FAN_TABLE_SKU1_FILE "/usr/share/swampd/config-sku1.json"
#define FAN_TABLE_SKU2_FILE "/usr/share/swampd/config-sku2.json"
#define FAN_TABLE_SKU3_FILE "/usr/share/swampd/config-sku3.json"
#define FAN_TABLE_FILE "/usr/share/swampd/config.json"
#define FAN_PATH "/sys/devices/platform/ahb/ahb:apb/f0083000.i2c/i2c-3/3-0075"
#define I2COOL_PATH "/sys/devices/platform/ahb/ahb:apb/\
f0083000.i2c/i2c-3/3-0077"

#define NUMBER_OF_SENSOR 2
#define TEMP_I2COOL_0_SPEC 32000
#define TEMP_I2COOL_1_SPEC 70000

#define DBUS_SET_PROPERTY_COMMAND "busctl set-property xyz.openbmc_project.Hwmon.external /xyz/openbmc_project/extsensors/margin/fleeting0 xyz.openbmc_project.Sensor.Value Value x "

std::string executeCmd(std::string cmd)
{
    char buffer[128];
    std::string result = "";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) 
    {
        return "failed";
    }

    while (!feof(pipe)) 
    {
        if (fgets(buffer, 128, pipe) != NULL)
        {
            result += buffer;
        }
    }
    pclose(pipe);

    return result;
}

std::string getFanDeviceAddr(int num)
{
    std::string cmd = "";
    std::string channel = "";

    if (num == 0)
    {
        channel = "/channel-2/";
    }
    else
    {
        channel = "/channel-3/";
    }

    cmd = "ls -la ";
    cmd += FAN_PATH;
    cmd += channel;
    cmd += " | grep 002c | awk '{print $9}'";
    cmd = executeCmd(cmd);
    if (cmd.compare("failed") == 0)
    {
        return "";
    }
    cmd.erase(cmd.end()-1, cmd.end());

    return cmd;
}

std::string getFan7To12Path()
{
    std::string fanPath = FAN_PATH;
    std::string cmd = "";
    std::string channel = "/channel-3/";
    std::string fanDeviceAddr = getFanDeviceAddr(1);

    if (fanDeviceAddr.empty())
    {
        return "";
    }
    fanPath += channel;
    fanPath += fanDeviceAddr;
    cmd = "ls -la ";
    cmd += fanPath;
    cmd += "/hwmon/ | grep hwmon | awk '{print $9}'";
    cmd = executeCmd(cmd);
    cmd.erase(cmd.end()-1, cmd.end());
    fanPath += "/hwmon/";
    fanPath += cmd;

    return fanPath;
}

std::string getTempI2coolPath(int num)
{
    std::string tempI2coolPath = I2COOL_PATH;
    std::string cmd = "";
    std::string channel = "";

    if (num == 0)
    {
        channel = "/channel-3/";
    }
    else if (num == 1)
    {
        channel = "/channel-4/";
    }

    cmd = "ls -la ";
    cmd += I2COOL_PATH;
    cmd += channel;
    cmd += " | grep 005c | awk '{print $9}'";
    cmd = executeCmd(cmd);
    cmd.erase(cmd.end()-1, cmd.end());
    tempI2coolPath += channel;
    tempI2coolPath += cmd;
    cmd = "ls -la ";
    cmd += tempI2coolPath;
    cmd += "/hwmon/ | grep hwmon | awk '{print $9}'";
    cmd = executeCmd(cmd);
    cmd.erase(cmd.end()-1, cmd.end());
    tempI2coolPath += "/hwmon/";
    tempI2coolPath += cmd;
    tempI2coolPath += "/temp1_input";

    return tempI2coolPath;
}

int main()
{
    /*** determine sku ***/
    std::string fanPath = "";
    std::string fanTableFile = FAN_TABLE_SKU1_FILE;
    std::fstream fanPwmFile;
    std::fstream fanRpmFile;
    bool fan7To12Control[6] = {false, false, false, false, false, false};
    int fanRpm = 0;

    /* TBD */
    for (int i = 0; i < 6; i++)
    {
        fanPath = getFan7To12Path();
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
        fanPath = getFan7To12Path();
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
            fan7To12Control[i] = true;
        }
    }

    if (fan7To12Control[0])
    {
        if (fan7To12Control[2])
        {
            fanTableFile = FAN_TABLE_SKU2_FILE;
        }
        else
        {
            fanTableFile = FAN_TABLE_SKU3_FILE;
        }
    }
    /* TBD */

    /*** generate config template ***/
    std::fstream defaultConfig;
    std::fstream outputConfig;
    std::string fan1To6DeviceAddr = getFanDeviceAddr(0);
    std::string fan7To12DeviceAddr = getFanDeviceAddr(1);
    std::string tmp = "";

    defaultConfig.open(fanTableFile, std::ios::in);
    outputConfig.open(FAN_TABLE_FILE, std::ios::out);
    if (defaultConfig)
    {
        while (std::getline(defaultConfig, tmp))
        {
            outputConfig << tmp << std::endl;
        }
    }
    else
    {
        return -1;
    }
    
    defaultConfig.close();
    outputConfig.close();

    if (!fan1To6DeviceAddr.empty() && !fan7To12DeviceAddr.empty())
    {
        tmp = "sed -i \"s/FAN1_TO_6/";
        tmp += fan1To6DeviceAddr;
        tmp += "/g\" ";
        tmp += FAN_TABLE_FILE;
        system(tmp.c_str());

        tmp = "sed -i \"s/FAN7_TO_12/";
        tmp += fan7To12DeviceAddr;
        tmp += "/g\" ";
        tmp += FAN_TABLE_FILE;
        system(tmp.c_str());
    }
    else
    {
        return -1;
    }
    

    /*** start fan control ***/
    system("/usr/bin/swampd &");

    /*** update fleeting0 temp ***/
    std::fstream sensorTempFile;
    std::string tempI2cool0Path = getTempI2coolPath(0);
    std::string tempI2cool1Path = getTempI2coolPath(1);
    int sensorRealTemp = 0;
    int sensorSpecTemp = 0;
    int sensorMarginTemp = 0;
    int marginTemp = -1;
    std::pair<std::string, int> sensorList[NUMBER_OF_SENSOR];

    sensorList[0] = std::make_pair(tempI2cool0Path, TEMP_I2COOL_0_SPEC);
    sensorList[1] = std::make_pair(tempI2cool1Path, TEMP_I2COOL_1_SPEC);

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
        tmp = DBUS_SET_PROPERTY_COMMAND + std::to_string(sensorMarginTemp);
        system(tmp.c_str());

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
