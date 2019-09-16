#ifndef O2_H
#define O2_H
// System status
const static int IDLE      = 0;
const static int STARTUP   = 1;
const static int ON        = 2;
const static int SHUTDOWN  = 3;
const static int STANDBY   = 4;

// Calibration status codes
const static int CAL_IDLE = 0;
const static int CAL_PROG  = 1;
const static int CAL_DONE  = 2;

// Flag codes
const static int FLAG_NONE  = 0;
const static int FLAG_CAL   = 1;
const static int FLAG_DONE  = 2; // calibration done, that is
const static int FLAG_OFF   = 3;

const int RR_SUCCESS = 1;
const int RR_FAIL = -1;
const int WR_SUCCESS = 0;
const int WR_FAIL = -1;

const int COND0 = 0; // Regular operation or unknown state
const int COND1 = 1; // Calibrate
const int COND2 = 2; // Reset calibration (write reset on sensor)
const int COND3 = 3; // Reset flag (clear 'calibration wanted' flag)
const int COND4 = 4; // Turn off
const int COND5 = 5; // Turn on

#endif
