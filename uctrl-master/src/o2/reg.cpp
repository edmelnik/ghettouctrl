#include "reg.h"

/*

  DONE parse protocol errors

  Response codes

  0  Success
  1 invalid response slave ID exception
  2 invalid response function exception
  3 response timed out exception
  4 invalid response CRC exception
  5 modbus proto illegal func
  6 modbus proto illegal data address
  7 modbus proto illegal data value
  8 modbus proto device failure

  4-20ma.io/ModbusMaster/group__constant.html#gaf69c1c360b84adc5bf1c7bbe071f1970

  This func expects int representations of hex error values and converts them 
  into errors in [-1, -8]
*/
int parseError(int e) {
    int ret;
    if(e >= 224 && e <= 227)
	ret = 0-(e % 223);
    else if(e >=1 && e <= 4)
	ret = 0-(e+4);
    return ret;
}

int readReg(int sensor, int reg, ModbusMaster *node) {
    int res;
    int val;
    res = node[sensor].readInputRegisters(reg, 1);
    if(res == 0){
	val = node[sensor].getResponseBuffer(0);
	node[sensor].clearResponseBuffer();
    }
    else{
	return parseError(res);
    }
    return val;
}

int writeReg(int sensor, int reg, int value, ModbusMaster *node) {
    int res, ret;
    res = node[sensor].writeSingleRegister(reg, value);
    if(res == 0)
	ret = res;
    else
	ret = parseError(res);
    return ret;
}
