#include "o2.h"

int handleSensor(int status, int cal, int flag){
    int res, cal_res, retval;
    if((status == IDLE || status == STANDBY) && flag != FLAG_OFF){ // Turn ON
	retval=COND5;
	if(cal==CAL_DONE)
	    flag=FLAG_DONE;
    }
    else if(flag == FLAG_CAL && cal != CAL_PROG &&
	    (status != STARTUP && status !=SHUTDOWN)){ // Calibrate
	retval=COND1;
    }
    else if(cal== CAL_DONE){ // Reset sensor
	retval=COND2;
    }
    else if(cal == CAL_IDLE && flag == FLAG_DONE){ // Reset calibration flag
	retval=COND3;
    }
    else if(flag == FLAG_OFF && (status == ON || status == STANDBY)){ // Manual Shutdown
	retval=COND4;
    }
    else{
	retval=COND0;
    }
    return retval;
}
