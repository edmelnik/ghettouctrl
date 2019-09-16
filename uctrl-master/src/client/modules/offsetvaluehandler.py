# This script continuously monitors client output to catch and update values that are required in pressure offsetting

import configparser

CONF = "conf/offsetvalues.conf"
config = configparser.ConfigParser()
config.read(CONF)

BARO_REG = 30016

last_cal_value = config['values']['last_cal_value']
prs_last_cal = config['values']['prs_last_cal']
baro = config['values']['baro']
humidity = config['values']['baro']

cal_prs = [0.0,0.0,0.0,0.0]

def getBaro(values):
    baro_avg, num_points = 0, 0
    for x in range(3, len(values)):
        datapoint = values[x].split('|')
        try:
            if int(datapoint[2]) >=0:
                baro_avg += int(datapoint[2])
                num_points += 1
        except:
            continue
    baro_avg /= num_points
    config['values']['baro'] = str(baro_avg)
    with open(CONF, 'w') as updated_conf:
        config.write(updated_conf, space_around_delimiters=False)
        
curr_prs = [0.0, 0.0, 0.0, 0.0]            
def handlePressure(values):
    for i in range(2, len(values)):
        try:
            curr_prs[i-2] = float(values[i])*68.94757
        except:
            continue
        
def writePrsValues():
    confstring = ""
    confstring=",".join("%.2f"%round(val, 2) for val in cal_prs)
    config['values']['prs_last_cal'] = confstring
    with open(CONF, 'w') as updated_conf:
        config.write(updated_conf, space_around_delimiters=False)

cal_active = [False, False, False, False]        
def handleO2(values):
    for i in range(2, len(values)):
        if values[i][:3] == "CAL" and cal_active[i-2] == False:
            cal_active[i-2] = True
        elif values[i][:3] != "CAL" and cal_active[i-2] == True:
            cal_active[i-2] = False
            cal_prs[i-2] = curr_prs[i-2]
            writePrsValues()
        
def sendData(values):
    global cal_active
    if values[1] == "O10" and values[2] == "R":
        if int(values[3].split('|')[1]) == BARO_REG:
            getBaro(values)
    elif values[1] == "PRS":
        handlePressure(values)
    elif values[1] == "OXY":
        handleO2(values)

