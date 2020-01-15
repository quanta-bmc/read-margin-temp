#include <string>

#define FAN_PATH "/sys/devices/platform/ahb/ahb:apb/f0083000.i2c/i2c-3/3-0075"
#define I2COOL_PATH "/sys/devices/platform/ahb/ahb:apb/\
f0083000.i2c/i2c-3/3-0077"

std::string getFanDeviceAddr(int num);

std::string getFan6To11Path();

std::string getTempI2coolPath(int num);
