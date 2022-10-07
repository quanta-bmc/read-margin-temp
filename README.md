# read-margin-temp

This daemon is to calculate and update margin temperatures for  
[fan PID control](https://github.com/openbmc/phosphor-pid-control).

At 1 second intervals, all input sensors are read, and for each thermal zone, the combined worst margin is calculated and output.  
The input source type (dbus, sys, file) and measurement unit (millidegree, degree, millimargin, margin) are configurable, for each input sensor.

After reading all input sensors, and converting units as necessary, the combined worst margin will be output.  
The output type is dbus, and the output unit is margin (degrees of thermal safety margin).

This daemon will check sensor threshold and functional dbus property.  
If functional dbus is false, which means driver is not update value to dbus, it will set NaN to sensor dbus value.

## Configuration File

The configuration file has "sensors" and "skus" parts.  
The "sensors" part contains all the information of sensors,
including paths for temperature reading and paths for max temperature.
The "skus" part has the list of all sku configurations.  
The detailed introduction is listed below.

# Sensor Configuration
```
"sensors" : [
    {
        "name": "cpu0",           /* name of the sensor */
        "unit": "millidegree",    /* temperature unit: millidegree, degree, millimargin, margin */
        "type": "dbus",           /* temperature reading path type: dbus, sys, file */
        "path": "/xyz/openbmc_project/sensors/temperature/cpu0",   /* temperature reading dbus or sys path */
        "parameters": {
            "type": "sys",        /* max temperature reading type: sys, file */
            "maxTemp": -1,        /* fixed max temperature, -1 for not fixed, unit: millidegree */
            "path": "/sys/devices/platform/ahb/ahb:apb/f0100000.peci-bus/peci-0/0-30/peci-cputemp.0/hwmon",
                                    /* max temperature reading /sys/devices path */
            "sysLabel": "Tjmax",  /* if sys, hwmon label for finding hwmon input file */
            "targetTemp": -1,     /* calibration target temperature, unit: millidegree */
            "targetTempOffset": -10000,   /* calibration target temperature's offset, unit: millidegree */
            "scalar": 1.0         /* scalar value */
        }
    }
]
```

# Sku Configuration
```
"skus" : [
    {
        "num": 1,           /* sku number */
        "zones": [
            {
                "id": 0,    /* zone id */
                "zoneSetpoint": 10000,  /* zone setpoint, unit: millimargin */
                "target": "/xyz/openbmc_project/extsensors/margin/real_fleeting0", /* target dbus path */
                "components": ["cpu0", "dimm1"]  /* sensor names list */
            },
            {
                "id": 1,    /* zone id */
                "zoneSetpoint": 10000,  /* zone setpoint, unit: millimargin */
                "target": "/xyz/openbmc_project/extsensors/margin/real_fleeting1", /* target dbus path */
                "components": ["cpu1", "dimm2"]  /* sensor names list */
            }
        ]
    }
]
```
