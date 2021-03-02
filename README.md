# read-margin-temp

This daemon is to calculate and update margin temperatures for
[fan PID control](https://github.com/openbmc/phosphor-pid-control).   
The margin temperatures are updated one by one with 1 second interval between each zone.  
The unit of margin temperature is millidegree.  
This daemon will check sensor threshold and functional dbus property.
If functional dbus is false, which means driver is not update value to dbus, 
it will set NaN to sensor dbus value.

## Configuration File

The configuration file has "sensors" and "skus" parts.  
The "sensors" part contains all the information of sensors,
including paths for temperature reading and paths for spec temperature.  
The "skus" part has the list of all sku configurations.  
The detailed introduction is listed below.

# Sensor Configuration

* "sensors" : [
    * {
        * "name": "cpu0",           /* name of the sensor */
        * "unit": "millidegree",    /* temperature unit: millidegree, degree, millimargin */
        * "type": "dbus",           /* temperature reading path type: dbus, sys */
        * "path": "/xyz/openbmc_project/sensors/temperature/cpu0",   /* temperature reading dbus or sys path */
        * "parameters": {
            * "type": "sys",        /* spec temperature reading type: sys, file */
            * "specTemp": -1,       /* fixed spec temperature, -1 for not fixed, unit: millidegree */
            * "path": "/sys/devices/platform/ahb/ahb:apb/f0100000.peci-bus/peci-0/0-30/peci-cputemp.0/hwmon",
                                    /* spec temperature reading /sys/devices path */
            * "sysLabel": "Tjmax",  /* for Tjmax, label-like spec reading */
            * "targetTemp": -1,     /* target temperature, unit: millidegree */
            * "targetTempOffset": -10000,   /* target temperature's offset, unit: millidegree */
            * "scalar": 1.0         /* scalar value */
        * }
    * }
* ]

# Sku Configuration

* "skus" : [
    * {
        * "num": 1,           /* sku number */
        * "zones": [
            * {
                * "id": 0,    /* zone id */
                * "zoneSetpoint": 10000,  /* zone setpoint, unit: millimargin */
                * "target": "/xyz/openbmc_project/extsensors/margin/real_fleeting0", /* target dbus path */
                * "components": ["cpu0", "dimm1"]
                            /* sensor names list */
            * },
            * {
                * "id": 1,    /* zone id */
                * "zoneSetpoint": 10000,  /* zone setpoint, unit: millimargin */
                * "target": "/xyz/openbmc_project/extsensors/margin/real_fleeting1", /* target dbus path */
                * "components": ["cpu1", "dimm2"]
                            /* sensor names list */
            * }
        * ]
    * }
* ]
