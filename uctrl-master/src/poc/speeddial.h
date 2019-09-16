/*

Header file for speed dial functions and their definitions

TODO Regdump on speed dial 5

*/

int speedDialCal(int data){
    int i;
    for(i=NUM_SENSORS-1; i>=0; i--)
	if(((data & (1 << i)) > 0) && status[NUM_SENSORS-1-i]>0)
	    flag[NUM_SENSORS-1-i] = FLAG_CAL;
    return 1;
}

int speedDialOn(int data){
    int i, sensor;
    for(i=NUM_SENSORS-1; i>=0; i--){
	sensor=NUM_SENSORS-1-i;
	if(((data & (1 << i)) > 0) &&
	   (status[sensor]==IDLE || status[sensor] == STANDBY))
	    flag[sensor] = FLAG_NONE;
    }
    return 1;
}

int speedDialOff(int data){
    int i, sensor;
    for(i=NUM_SENSORS-1; i>=0; i--){
	sensor=NUM_SENSORS-1-i;
	if((data & (1 << i)) > 0)
	    flag[sensor] = FLAG_OFF;
    }
    return 1;
}

int speedDialToggle(int data){
    int i, sensor;
    for(i=NUM_SENSORS-1; i>=0; i--){
	sensor=NUM_SENSORS-1-i;
	if((data & (1 << i)) > 0){
	    if(status[sensor]==IDLE || status[sensor] == STANDBY)
		flag[sensor] = FLAG_NONE;
	    else if(status[sensor] == ON)
		flag[sensor] = FLAG_OFF;
	}
    }
}
