#include <check.h>
#include <stdio.h>
#include "o2.h"
#include "handler.h"

/* int handleSensor(int status, int cal, int flag, int wr_success, int rr_success) */

// Regular operation: unwanted turn off
START_TEST(ht_regular){
    ck_assert_int_eq(handleSensor(IDLE, CAL_IDLE, FLAG_NONE), COND5); // Pull up to ON
    ck_assert_int_eq(handleSensor(STARTUP, CAL_IDLE, FLAG_NONE), COND0);
    ck_assert_int_eq(handleSensor(ON, CAL_IDLE, FLAG_NONE), COND0);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_IDLE, FLAG_NONE), COND0);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_IDLE, FLAG_NONE), COND5);	
}
END_TEST

// Calibration wanted: calibrate when possible
START_TEST(ht_cal_idle_flag_cal){
    ck_assert_int_eq(handleSensor(IDLE, CAL_IDLE, FLAG_CAL), COND5); // Pull up to ON
    ck_assert_int_eq(handleSensor(STARTUP, CAL_IDLE, FLAG_CAL), COND0); // Do nothing
    ck_assert_int_eq(handleSensor(ON, CAL_IDLE, FLAG_CAL), COND1);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_IDLE, FLAG_CAL), COND0); // Do nothing
    ck_assert_int_eq(handleSensor(STANDBY, CAL_IDLE, FLAG_CAL), COND5);	 // Pull up to ON
}
END_TEST

START_TEST(ht_cal_idle_flag_done){
    ck_assert_int_eq(handleSensor(IDLE, CAL_IDLE, FLAG_DONE), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_IDLE, FLAG_DONE), COND3);
    ck_assert_int_eq(handleSensor(ON, CAL_IDLE, FLAG_DONE), COND3);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_IDLE, FLAG_DONE), COND3);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_IDLE, FLAG_DONE), COND5);
}
END_TEST

START_TEST(ht_cal_idle_flag_off){
    ck_assert_int_eq(handleSensor(IDLE, CAL_IDLE, FLAG_OFF), COND0); // Do nothing
    ck_assert_int_eq(handleSensor(STARTUP, CAL_IDLE, FLAG_OFF), COND0); // Do nothing
    ck_assert_int_eq(handleSensor(ON, CAL_IDLE, FLAG_OFF), COND4);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_IDLE, FLAG_OFF), COND0); // Do nothing
    ck_assert_int_eq(handleSensor(STANDBY, CAL_IDLE, FLAG_OFF), COND4);	
}
END_TEST

// Cal prog
START_TEST(ht_cal_prog_flag_none){
    ck_assert_int_eq(handleSensor(IDLE, CAL_PROG, FLAG_NONE), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_PROG, FLAG_NONE), COND0);
    ck_assert_int_eq(handleSensor(ON, CAL_PROG, FLAG_NONE), COND0);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_PROG, FLAG_NONE), COND0);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_PROG, FLAG_NONE), COND5);	
}
END_TEST

START_TEST(ht_cal_prog_flag_cal){
    ck_assert_int_eq(handleSensor(IDLE, CAL_PROG, FLAG_CAL), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_PROG, FLAG_CAL), COND0);
    ck_assert_int_eq(handleSensor(ON, CAL_PROG, FLAG_CAL), COND0);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_PROG, FLAG_CAL), COND0);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_PROG, FLAG_CAL), COND5);	
}
END_TEST

START_TEST(ht_cal_prog_flag_done){
    ck_assert_int_eq(handleSensor(IDLE, CAL_PROG, FLAG_DONE), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_PROG, FLAG_DONE), COND0);
    ck_assert_int_eq(handleSensor(ON, CAL_PROG, FLAG_DONE), COND0);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_PROG, FLAG_DONE), COND0);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_PROG, FLAG_DONE), COND5);	
}
END_TEST

START_TEST(ht_cal_prog_flag_off){
    ck_assert_int_eq(handleSensor(IDLE, CAL_PROG, FLAG_OFF), COND0);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_PROG, FLAG_OFF), COND0);
    ck_assert_int_eq(handleSensor(ON, CAL_PROG, FLAG_OFF), COND4);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_PROG, FLAG_OFF), COND0);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_PROG, FLAG_OFF), COND4);	
}
END_TEST

// cal done
START_TEST(ht_cal_done_flag_none){
    ck_assert_int_eq(handleSensor(IDLE, CAL_DONE, FLAG_NONE), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_DONE, FLAG_NONE), COND2);
    ck_assert_int_eq(handleSensor(ON, CAL_DONE, FLAG_NONE), COND2);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_DONE, FLAG_NONE), COND2);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_DONE, FLAG_NONE), COND5);
}
END_TEST

START_TEST(ht_cal_done_flag_cal){
    ck_assert_int_eq(handleSensor(IDLE, CAL_DONE, FLAG_CAL), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_DONE, FLAG_CAL), COND2);
    ck_assert_int_eq(handleSensor(ON, CAL_DONE, FLAG_CAL), COND1);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_DONE, FLAG_CAL), COND2);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_DONE, FLAG_CAL), COND5);
}
END_TEST

START_TEST(ht_cal_done_flag_off){
    ck_assert_int_eq(handleSensor(IDLE, CAL_DONE, FLAG_OFF), COND2);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_DONE, FLAG_OFF), COND2);
    ck_assert_int_eq(handleSensor(ON, CAL_DONE, FLAG_OFF), COND2);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_DONE, FLAG_OFF), COND2);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_DONE, FLAG_OFF), COND2);
}
END_TEST

START_TEST(ht_cal_done_flag_done){
    ck_assert_int_eq(handleSensor(IDLE, CAL_DONE, FLAG_DONE), COND5);
    ck_assert_int_eq(handleSensor(STARTUP, CAL_DONE, FLAG_DONE), COND2);
    ck_assert_int_eq(handleSensor(ON, CAL_DONE, FLAG_DONE), COND2);
    ck_assert_int_eq(handleSensor(SHUTDOWN, CAL_DONE, FLAG_DONE), COND2);
    ck_assert_int_eq(handleSensor(STANDBY, CAL_DONE, FLAG_DONE), COND5);
}
END_TEST

Suite *handler_suite(){
    Suite *s;
    TCase *tc_core;

    s = suite_create("Sensor handler");
    tc_core =  tcase_create("Core");

    tcase_add_test(tc_core, ht_regular); // cal_idle_flag_none
    tcase_add_test(tc_core, ht_cal_idle_flag_cal);
    tcase_add_test(tc_core, ht_cal_idle_flag_off);
    tcase_add_test(tc_core, ht_cal_idle_flag_done); 
    tcase_add_test(tc_core, ht_cal_prog_flag_none);
    tcase_add_test(tc_core, ht_cal_prog_flag_cal);
    tcase_add_test(tc_core, ht_cal_prog_flag_done);
    tcase_add_test(tc_core, ht_cal_prog_flag_off);
    tcase_add_test(tc_core, ht_cal_done_flag_none);
    tcase_add_test(tc_core, ht_cal_done_flag_cal);
    tcase_add_test(tc_core, ht_cal_done_flag_off);
    tcase_add_test(tc_core, ht_cal_done_flag_done);

    
    suite_add_tcase(s, tc_core);

    return s;
}

int main(){
    int fails;
    Suite *s;
    SRunner *sr;

    s = handler_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    fails=srunner_ntests_failed(sr);
    srunner_free(sr);
    return 0;
}
