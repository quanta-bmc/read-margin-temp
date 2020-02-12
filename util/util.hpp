#pragma once

#include <map>
#include <vector>
#include <string>

#include "conf.hpp"

/**
 * Determine sku type.
 *
 * @return Sku type of the system.
 */
int getSkuNum();

/**
 * Get sensor temp from dbus.
 *
 * @param[in] sensorDbusPath - sensor dbus path.
 * @return Sensor temp.
 */
int getSensorDbusTemp(std::string sensorDbusPath);

/**
 * Get spec temp.
 *
 * @param[in] config - sensor config.
 * @return Spec temp of the sensor.
 */
int getSpecTemp(struct conf::sensorConfig config);

/**
 * Get dbus service name.
 *
 * @param[in] dbusPath - sensor dbus path.
 * @return Dbus service name.
 */
std::string getService(const std::string dbusPath);

/**
 * Update margin temp of zone zoneNum.
 *
 * @param[in] zoneNum - zone number.
 * @param[in] marginTemp - margin temp.
 */
void updateDbusMarginTemp(int zoneNum, int64_t marginTemp);

/**
 * Calculate and update margin temp every second.
 *
 * @param[in] skuConfig - sku config.
 * @param[in] sensorConfig - sensor config.
 */
void updateMarginTempLoop(
    std::map<int, std::vector<std::string>> skuConfig,
    std::map<std::string, struct conf::sensorConfig> sensorConfig);
