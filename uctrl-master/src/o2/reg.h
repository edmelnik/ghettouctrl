#ifndef O2_REG
#define O2_REG

/* #include "o2.h" */
#include <ModbusMaster.h>

int parseError(int e);
int readReg(int sensor, int reg, ModbusMaster *node);
int writeReg(int sensor, int reg, int value, ModbusMaster *node);

#endif
