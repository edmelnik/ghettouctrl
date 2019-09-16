#ifndef O2_H
#define O2_H

#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include <String.h>

#define NUM_SENSORS 4
#define BAUD 9600

// SST Registers
// Input regs 
#define O2AVG_REG  30001
#define PPO2_REG   30014
#define STATUS_REG 30004
#define ERR_REG    30005
#define CALSTS_REG 30018
// Holding regs
#define ONOFF_REG   40001
#define CLCTRL_REG  40004 // Calibration control
#define ADDR_REG    40006
#define ERR_CLR_REG 40002

// Starting PIN on the microcontroller
const static unsigned int START_PIN PROGMEM = 4;
// Delay for periodic checks (status, calibration et cetera)
const static unsigned int CHK_DELAY = 20;

// System status
const static int IDLE     PROGMEM = 0;
const static int STARTUP  PROGMEM = 1;
const static int ON       PROGMEM = 2;
const static int SHUTDOWN PROGMEM = 3;
const static int STANDBY  PROGMEM = 4;

// Calibration status codes
const static int CAL_IDLE PROGMEM = 0;
const static int CAL_PROG PROGMEM = 1;
const static int CAL_DONE PROGMEM = 2;

// Flag codes
const static int FLAG_NONE PROGMEM = 0;
const static int FLAG_CAL  PROGMEM = 1;
const static int FLAG_DONE PROGMEM = 2;
const static int FLAG_OFF  PROGMEM = 3;
 
ModbusMaster *node = malloc(sizeof(ModbusMaster)*4);
SoftwareSerial modbus(4, 5);

/*
  Status < 0 means there's an active error code in err[] for the sensor
  Error == 0 means that there's sensor status value = valid

  flag[] tracks the calibration demands from the interrupt pins,
    while cal tracks the actual calibration state of the boards
  flag[i] == FLAG_CAL means that calibration is in process for sensor i
  this calibration could be DONE, PROG or IDLE.. cal[i] would hold
    that info
*/
int status[NUM_SENSORS], cal[NUM_SENSORS], flag[NUM_SENSORS] = {FLAG_NONE};

#endif
