# read-margin-temp

This daemon is to calculate and update margin temperatures for
[fan PID control](https://github.com/openbmc/phosphor-pid-control). The margin
temperatures are updated one by one with 1 second interval between each zone.
The unit of margin temperature is millidegree.

## Configuration File

The configuration file has "sensors" and "skus" parts. The "sensors" part
contains all the information of sensors, including paths for temperature
reading and paths for spec temperature. The "skus" part has the list of all
sku configurations. The detailed introduction is listed below.

# Sensor Configuration

"sensors" : [
    {
        "name": "cpu0",         /* name of the sensor */
        "unit": "millidegree",  /* temperature unit: millidegree, degree */
        "pathType": "dbus",     /* temperature reading path type: dbus, sys */
        "dbusPath": "/xyz/openbmc_project/sensors/temperature/cpu0",
                                /* temperature reading dbus path */
        "sysPath": "",          /* temperature reading /sys/devices path */
        "sysLabel": "",         /* for Tjmax-like reading */
        "sysInput": "",         /* for temp1_input-like reading */
        "sysChannel": -1,       /* sys path channel number, -1: no channel */
        "sysReg": "",           /* sys path register */
        "offset": 0,            /* offset value */
        "spec": {
            "type": "sys",      /* spec temperature reading type: sys, file */
            "specTemp": -1,     /* fixed spec temperature, -1 for not fixed */
            "path": "/sys/devices/platform/ahb/ahb:apb/f0100000.peci-bus/peci-0/0-30/peci-cputemp.0/hwmon",
                                /* spec temperature reading /sys/devices path */
            "sysLabel": "Tjmax",
                                /* for Tjmax, label-like spec reading */
            "sysInput": "",     /* for temp1_input like spec reading */
            "sysChannel": -1,   /* sys path channel number, -1 means no channel */
            "sysReg": ""        /* sys path register */
        }
    }
]

# Sku Configuration

"skus" : [
    {
        "num": 1,           /* sku number */
        "zones": [
            {
                "id": 0,    /* zone id */
                "components": ["cpu0", "dimm1"]
                            /* sensor names list */
            },
            {
                "id": 1,    /* zone id */
                "components": ["cpu1", "dimm2"]
                            /* sensor names list */
            }
        ]
    }
]
