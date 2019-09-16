#ifndef REG_H
#define REG_H

/* #include "o2.h" */
#include <ModbusMaster.h>

int parseError(int e);
int readReg(int sensor, int reg, ModbusMaster *node);
int writeReg(int sensor, int reg, int value, ModbusMaster *node);

#endif
