# This script provides function(s) that handle offsetting

import configparser

# placeholder for conf location that is the abspath on uctrlservice pi
CONF = "/home/alarm/run/conf/offsetvalues.conf"
config = configparser.ConfigParser()

config.read(CONF)
last_cal_value = float(config['values']['last_cal_value'])
prs_last_cal = [float(f) for f in config['values']['prs_last_cal'].split(",")]
baro = float(config['values']['baro'])
humidity = float(config['values']['humidity'])

CHK_DELAY = 100
NUM_SENSORS = 4

k = 1

def refreshValues():
    config.read(CONF)
    global last_cal_value, prs_last_cal, baro, humidity
    last_cal_value = float(config['values']['last_cal_value'])
    prs_last_cal = [float(f) for f in config['values']['prs_last_cal'].split(",")]
    # prs_last_cal = config['values']['prs_last_cal']
    baro = float(config['values']['baro'])
    humidity = float(config['values']['humidity'])

# parameters are values for 1 sensors
def getOffset(curr_ppO2, curr_pressure, sensor_id):
    global k
    val = (curr_ppO2+(prs_last_cal[sensor_id]*last_cal_value))/(baro+curr_pressure-humidity)
    val*=100
    k+=1
    if k == CHK_DELAY:
        refreshValues()
    k%=CHK_DELAY    
    return val

