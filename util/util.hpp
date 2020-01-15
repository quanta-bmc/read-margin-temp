#pragma once

#define NUMBER_OF_SENSOR 4
#define TEMP_CPU_0_SPEC 0
#define TEMP_CPU_1_SPEC 0
#define TEMP_DIMM_0_SPEC 0
#define TEMP_DIMM_1_SPEC 0
#define TEMP_I2COOL_0_SPEC 32000
#define TEMP_I2COOL_1_SPEC 70000
#define TEMP_I2COOL_2_SPEC 70000
#define TEMP_I2COOL_3_SPEC 70000
#define TEMP_PCIE_0_SPEC 0
#define TEMP_PCIE_1_SPEC 0
#define TEMP_PCIE_2_SPEC 0
#define TEMP_PCIE_3_SPEC 0
#define TEMP_PCIE_4_SPEC 0
#define TEMP_PCIE_5_SPEC 0
#define TEMP_PCIE_6_SPEC 0
#define TEMP_PCIE_7_SPEC 0
#define TEMP_GPU_SPEC 0
#define TEMP_LOW_PROFILE_0_SPEC 0
#define TEMP_LOW_PROFILE_1_SPEC 0
#define TEMP_LOW_PROFILE_2_SPEC 0
#define TEMP_LOW_PROFILE_3_SPEC 0

constexpr auto dbusSetPropertyCommand =
    "busctl set-property xyz.openbmc_project.Hwmon.external /xyz/openbmc_project/extsensors/margin/fleeting0 xyz.openbmc_project.Sensor.Value Value x ";

int getSkuNum();

void updateMarginTempLoop();
