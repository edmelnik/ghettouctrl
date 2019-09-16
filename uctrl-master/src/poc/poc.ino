
#include <Wire.h>
#include "o2.h"
#include "reg.h"
#include "datagram.h"
#include "hsc_ssc_i2c.h"
#include "speeddial.h"

//=============================Pressure Vars============================
#define MAX_SENSORS 5
// Constant mux address
#define MUXADDR 0x70
// see hsc_ssc_i2c.h for a description of these values
// these defaults are valid for the HSCMRNN030PA2A3 chip
#define SLAVE_ADDR 0x28
#define OUTPUT_MIN 0
#define OUTPUT_MAX 0x3fff       // 2^14 - 1
#define P_MIN0 -1
#define P_MAX0 1
#define P_MIN1 -5
#define P_MAX1 5
//======================================================================
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

//=============================Oxygen Funcs=============================
void handleSensor(int i){
    int res, cal_res;
    status[i] = readReg(i, STATUS_REG, node); delay(4);
    cal[i] = readReg(i, CALSTS_REG, node); delay(4);
    
    if(flag[i] == FLAG_CAL && cal[i] == CAL_IDLE){ // Calibrate
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
    else if((status[i] == IDLE || status[i] == STANDBY) && flag[i] == FLAG_NONE){ // Turn ON
	res = writeReg(i, ONOFF_REG, 1, node); delay(4);
	if(res < 0)
	    status[i] = res;
	else
	    status[i] = readReg(i, STATUS_REG, node); // TODO clear errors
    }
}

/*

  Data, error, status, calibration output handler used by loop() and calibrate()
  Cal out is a boolean: 1 if called by calibration and only prints cal status (does not set data)

*/
int getVal(int sensor, char *output, unsigned int cal_out=0){
    int retval, data;
    char o2_str[20], errstr[10] = "ERR", *errval, statstr[10] = "STS";
    char calstr[10] = "CAL";
       
    errval = malloc(10);
    
    if(flag[sensor]>FLAG_NONE && flag[sensor]<FLAG_OFF){ // Get calibration status
	strcat(calstr, itoa(cal[sensor], errval, 10));
	strcpy(output, calstr);
	retval = 1;
    }
    else if(status[sensor] == ON){    // Get O2 data
	data = readReg(sensor, PPO2_REG, node);
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
    int i, buf_ptr=0, sensor, regval, buf_sz = 100, retval, rw, handleflag=0;
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
	buf_ptr +=snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "%c|%u|%u|%d ",
			  _cmd.rw, _cmd.reg, _cmd.data, retval);
	Serial.println(buffer);	
	free(buffer);
	handleflag=1;
    }
    return handleflag;
}


//======================================================================


//=============================Pressure Funcs===========================
int get_i2c(int mux_addr, int mux_channel, int device_addr,
	    int min, int max, char* pressure_out){

    char errstr[20] = "ERR", *errval;
    errval = malloc(3);
    
    Wire.beginTransmission(mux_addr);
    Wire.write(1 << mux_channel);
    Wire.endTransmission();

    unsigned long now = millis();
    struct cs_raw ps;
    char p_str[10], t_str[10], p_raw_str[10];
    uint8_t el;
    float p, t;
    el = ps_get_raw(device_addr, &ps);
    if(el <= 4 && el >= 1){
	strcat(errstr, itoa(el, errval, 10));
	strcpy(pressure_out, errstr);
	free(errval);
	return -1;
    }
    else{
	ps_convert(ps, &p, &t, OUTPUT_MIN, OUTPUT_MAX, min, max);
	dtostrf(p, 2, 3, p_str); 
	dtostrf(t, 2, 3, t_str);	
	strcpy(pressure_out, p_str);
	free(errval);
	return 1;
    }           
}

//======================================================================


void setup(){    
    int i, curr_pin, SST_addr;
    
    Serial.setTimeout(2000);
    Serial.begin(BAUD); // To USB output
    modbus.begin(BAUD); // To RS485 bus
    Wire.begin();
    
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

    // At this point ideally all sensors are turned on.
    // Sensors that have an active error code and not turned on
    //  should be handled appropriately in loop()
}

int k=0, curr_iter = 0;

void loop(){
    char *buffer;
    //=================================Oxygen operation=============================
    if(curr_iter == 0){ // Oxygen data       
	int i, buf_ptr = 0, data, retval, handle_flag[] = {0,0,0,0}, hf, buf_sz = 50;
	char *buffer, *output;    

	buffer = malloc(buf_sz);
	buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "OXY ");
	for(i=0; i<NUM_SENSORS; i++){
	    output = malloc(30);
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
	
	    buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr,
				" %s ", output);
	    free(output);
	    delay(4);
	}
	Serial.println(buffer);
	free(buffer);
    
//	if(k==CHK_DELAY){
//	    hf=handleCommands();
//	    for(i=0; i< NUM_SENSORS; i++)
//		if(flag[i] == FLAG_CAL || flag[i] == FLAG_OFF || hf==1)
//		    handle_flag[i] = 1;	
//	}
    
	for(i=0; k==CHK_DELAY && i<NUM_SENSORS; i++)
	    if(handle_flag[i] == 1)
		handleSensor(i);
    
	k+=1;
	k%=(CHK_DELAY+1);
    }
    //=================================Pressure operation=============================
    else if(curr_iter == 1){
	char *pressure;
	int i, retval, buf_ptr = 0, buf_sz=50;

	buffer = malloc(buf_sz);
	pressure = malloc(10);
    	buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr, "PRS ");
	// indexed by 1 to correspond with mux_channel
	for(i=0; i<MAX_SENSORS; i++){
	    retval = get_i2c(MUXADDR, i, SLAVE_ADDR,
			     P_MIN1, P_MAX1, pressure);
	    buf_ptr += snprintf(buffer+buf_ptr, buf_sz-buf_ptr,
				" %s ", pressure);
	}
	
	Serial.println(buffer);
	free(buffer);
	free(pressure);
    }
    
    curr_iter = ((curr_iter+1)%2);
}
