#pragma once

#include <map>
#include <vector>
#include <string>

#include "conf/conf.hpp"

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
double getSensorDbusTemp(std::string sensorDbusPath);

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
void updateDbusMarginTemp(int zoneNum, int64_t marginTemp, std::string targetpath);

/**
 * Calculate and update margin temp every second.
 *
 * @param[in] skuConfig - sku config.
 * @param[in] sensorConfig - sensor config.
 */
void updateMarginTempLoop(
    conf::skuConfig skuConfig,
    std::map<std::string, conf::sensorComponents> sensorConfig);
