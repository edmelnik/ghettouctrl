/*
  TODO Implement regdump on command ID 5 (maybe use the older regdump files as headers?)
  TODO String construction in getVal() should be optimized
    - Base strings should be global and in progmem, these strings should be copied as needed in getVal
  DONE Combine o2 and pressure into a single grand PoC
  DONE calibration routine
    DONE Solve problem in note (1)
  DONE Read multiple registers and parse the response buffer
  - Select relevent values (see comment above func loop)
  DONE Format the output values in a similar manner to how the pressure is being formatted
  - Might be possible to use the output function as a header file common to both
  DONE Better error checking and overall control flow
  - Periodically monitor status and check expected O2 values
  - O2 values could be modelled to make intelligent calibration decisions

  Will the ~ PWM pins with this? Should any strange and unexpected errors occur in the near future, blame this

NOTES
1. The input serial buffer does not seem to be cleared after the read, which is probably the first command works as expected while the others are the same as the first command. 
  - Worth trying serial.end at the end of handleCommands()
*/

#include "o2.h"
#include "reg.h"
#include "datagram.h"
#include "speeddial.h"

void handleSensor(int i){
    int res, cal_res;
    status[i] = readReg(i, STATUS_REG, node); delay(4);
    cal[i] = readReg(i, CALSTS_REG, node); delay(4);
    
    if((status[i] == IDLE || status[i] == STANDBY) && flag[i] == FLAG_NONE){ // Turn ON
	res = writeReg(i, ONOFF_REG, 1, node); delay(4);
	if(res < 0)
	    status[i] = res;
	else
	    status[i] = readReg(i, STATUS_REG, node); // TODO clear errors
	if(cal[i] == CAL_DONE)
	    flag[i] = CAL_DONE;
    }
    else if(flag[i] == FLAG_CAL && cal[i] == CAL_IDLE){ // Calibrate
	res = writeReg(i, CLCTRL_REG, 1, node); delay(4);
	if(res < 0)
	    status[i] = res;
	else
	    cal[i] = readReg(i, CALSTS_REG, node); delay(4);
    }
    else if(cal[i] == CAL_DONE){ // Reset sensor
	res = writeReg(i, CLCTRL_REG, 2, node); delay(4);
	if(res < 0)
	    status[i] = res;
	else{
	    cal[i] = readReg(i, CALSTS_REG, node); delay(10);	    
	    flag[i] = cal[i];
	}
    }
    else if(cal[i] == CAL_IDLE && flag[i] == FLAG_DONE){ // Reset flag
	flag[i] = 0;
    }
    else if(flag[i] == FLAG_OFF){ // Manual Shutdown
	res = writeReg(i, ONOFF_REG, 0, node); delay(4);
	if(res < 0)
	    status[i] = res;
	else
	    status[i] = readReg(i, STATUS_REG, node);
    }
    else{ // No action taken, just return
	return;
    }
}

/*

  Data, error, status, calibration output handler used by loop() and calibrate()
  Cal out is a boolean: 1 if called by calibration and only prints cal status (does not set data)

*/
int getVal(int sensor, char *output, unsigned int cal_out=0){
    int retval, data;
    char o2_str[10], errstr[10] = "ERR", *errval, statstr[10] = "STS";
    char calstr[10] = "CAL";
       
    errval = malloc(5);
    
    if(flag[sensor]>FLAG_NONE && flag[sensor]<FLAG_OFF){ // Get calibration status
	strcat(calstr, itoa(cal[sensor], errval, 10));
	strcpy(output, calstr);
	retval = 1;
    }
    else if(status[sensor] == ON){    // Get O2 data
	data = readReg(sensor, O2AVG_REG, node);
	if(data < 0){                 // Failed to get data, output ERR
	    strcat(errstr, itoa(data*-1, errval, 10));
	    strcpy(output, errstr);	    
	    retval = -1;
	}
	else{                         // Got data
	    dtostrf(data, 4, 0, o2_str);
	    strcpy(output, o2_str);
	    retval = 1;
	}
    }
    else{
	if(status[sensor] >= 0){     // Status is not ON - get status
	    strcat(statstr, itoa(status[sensor], errval, 10));
	    strcpy(output, statstr);
	    // Return failure only if sensor was not manually off
	    if(flag[sensor] == FLAG_OFF)
		retval = 1;
	    else
		retval = -2;
	}
	else{                       // Error status
	    strcat(errstr, itoa(status[sensor]*-1, errval, 10));
	    strcpy(output, errstr);
	    retval = -3;
	}
    }
    free(errval);
    return retval;
}

int bridge(struct cmd _cmd){
    int i, buf_ptr=0, sensor, regval, buf_sz = 100, retval, rw, handleflag;
    char *buffer, *locbuf, garbage;
    buffer = malloc(buf_sz);

    rw = ((int)_cmd.rw)%48;
    
    buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "O10 ");
    if(rw == 0){ // Read
	buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "R ");
	for(i=NUM_SENSORS-1; i>=0; i--){
	    if((_cmd.data & (1 << i)) > 0){
		sensor = NUM_SENSORS-1-i;
		regval = readReg(sensor, _cmd.reg, node); delay(4);
		buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr,
				    "%u|%u|%d ",
				    sensor+1, _cmd.reg, regval);
	    }
	}
	Serial.println(buffer);	
	free(buffer);
	Serial.flush();
	handleflag=0;
    }
    else if(rw > 0){ // Write	
	buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "W ");
	sensor=rw-1;
	retval = writeReg(sensor, _cmd.reg, _cmd.data, node);
	buf_ptr+=snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "%c|%u|%u|%d ",
			  _cmd.rw, _cmd.reg, _cmd.data, retval);
	Serial.println(buffer);	
	free(buffer);
	handleflag=1;
    }
    return handleflag;
}

int handleCommands(){
    int handleflag;
    char *readval, checksum;
    struct cmd _cmd;
    readval = malloc(13);
    Serial.readBytes(readval, 12);
    
    checksum = getChecksum(readval);
    parseCommand(&_cmd, readval);
    if(checksum != _cmd.checksum)
	_cmd.command = 0;
    switch(_cmd.command){
    case 0:
	break;
    case 1: // Command ID 1: Calibration	
	handleflag=speedDialCal(_cmd.data);
	break;
    case 2: // Command ID 2: On/off
	handleflag=speedDialOn(_cmd.data);
	break;
    case 3:
	handleflag=speedDialOff(_cmd.data);
	break;
    case 4:
	handleflag=speedDialToggle(_cmd.data);
	break;
    case 10: // Command ID 10: Bridge mode
	handleflag=bridge(_cmd);
	break;
    }
    free(readval);
    return handleflag;
}

void setup(){    
    int i, curr_pin, SST_addr;

    Serial.setTimeout(2000);
    Serial.begin(BAUD); // To USB output
    modbus.begin(BAUD); // To RS485 bus
    
    // Actual sensor addresses are indexed by 1, but
    // once sensor addresses are configured in node[], they should indexed by 0
    for(i=0; i<NUM_SENSORS; i++){
    	SST_addr = i+1;
	node[i] = *(new ModbusMaster);
	node[i].begin(SST_addr, modbus);
    }
    
    // If sensor is idle, turn it on
    for(i=0; i<NUM_SENSORS; i++)
	handleSensor(i);

    pinMode(12, INPUT_PULLUP);
    // At this point ideally all sensors are turned on.
    // Sensors that have an active error code and not turned on
    //  should be handled appropriately in loop()
}

int k=0;

void loop(){
    int i, buf_ptr = 0, data, retval, handle_flag[] = {0,0,0,0}, hf;    
    char *buffer, *output;    

    buffer = malloc(50);
    buf_ptr += snprintf(buffer+buf_ptr, 50-buf_ptr, "OXY ");
    for(i=0; i<NUM_SENSORS; i++){
	output = malloc(20);
	if(flag[i] == FLAG_OFF && status[i] == ON){ // Flagged for manual shutdown
	    retval = getVal(i, output, 0);
	    handle_flag[i] = 1;
	}
	if(flag[i] > FLAG_NONE && flag[i] < FLAG_OFF){ // Flagged for calibration
	    retval = getVal(i, output, 1);
	    handle_flag[i] = 1;
	}
	else{
	    retval = getVal(i, output, 0); // Not flagged; try to get data
	    if(retval < 0)
		handle_flag[i] = 1;
	}
	
	buf_ptr += snprintf(buffer+buf_ptr, 50-buf_ptr,
			    " %s ", output);
	free(output);
	delay(4);
    }
    Serial.println(buffer);
    free(buffer);
    
    if(k==CHK_DELAY && Serial.available()){
	hf=handleCommands();
	for(i=0; i< NUM_SENSORS; i++)
	    if(flag[i] == FLAG_CAL || flag[i] == FLAG_OFF || hf==1)
		handle_flag[i] = 1;	
    }
    
    for(i=0; k==CHK_DELAY && i<NUM_SENSORS; i++)
	if(handle_flag[i] == 1)
	    handleSensor(i);
    
    k+=1;
    k%=(CHK_DELAY+1);
}

