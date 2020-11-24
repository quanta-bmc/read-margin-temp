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
double getSensorDbusTemp(std::string sensorDbusPath);

/**
  * Get spec temp.
  *
  * @param[in] config - sensor config.
  * @return Spec temp of the sensor.
  */
double getSpecTemp(struct conf::SensorConfig config);

/**
  * Calculate margin offset value.
  *
  * @param[in] setPointInt - zone setpoint, integer millidegrees.
  * @param[in] scalar - sensor scalar, floating-point, this is unitless.
  * @param[in] maxTemp - sensor max temp, floating-point degrees.
  * @param[in] targetTempInt - sensor target temp, integer millidegrees.
  * @param[in] targetOffsetInt - sensor target temp offset, integer millidegrees.
  * @return Margin offset.
  */
double calOffsetValue(int setPointInt, double scalar, double maxTemp, int targetTempInt, int targetOffsetInt);

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
    conf::SkuConfig skuConfig,
    std::map<std::string, struct conf::SensorConfig> sensorConfig);
