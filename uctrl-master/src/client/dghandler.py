#!/bin/python

import sys
import serial
import time
import configparser
import logging
from systemd import journal

#### Motor speed constants
r_fullstep = 200
r_microstep = 256
ADDRESS = 1
TYPENUM = 1
PULSE_DIV = 0

#### Configured high/low medium speeds
LOW = 100
MED = 200
HIGH = 300

CONF = "conf/dghandler.conf"
pipe = "/srv/http/cgi-bin/dghandler.pipe"
BAUD = 9600

logger = logging.getLogger('client_logger')
logger.addHandler(journal.JournalHandler())
logger.setLevel(logging.INFO)

#############################################
##
## Helper funcs to convert commands to binary
##
def hexstr(n, pad, serialized=False):
    begin='\\x'
    hex_n=('%0.'+str(pad)+'x')%n
    if serialized:
        return begin+hex_n
    else:
        return hex_n
    
def getChecksum(cmd):
    ck=0
    for b in cmd:
        ck+=ord(b)
    ck%=16
    return hexstr(ck, 1)

def getMotorBinCommand(address, cmdnum, typenum, motornum, value):
    cmd= hexstr(address, 2) + hexstr(cmdnum, 2) + \
        hexstr(typenum, 2)+hexstr(motornum, 2) + hexstr(value, 8)
    cmd_bytes = bytearray.fromhex(cmd)
    ck = cmd_bytes[0]
    for i in range(1, len(cmd_bytes)):
        ck+=cmd_bytes[i]
    cmd_bytes.append(ck)
    k=""
    for x in cmd_bytes:
        k+="\\x%x"%x
    return cmd_bytes

def getOxyBinCommand(cmd, rw, reg, data):
    command=hexstr(cmd, 2)+hexstr(rw, 1)+hexstr(reg, 4)+hexstr(data, 4)
    command+=getChecksum(command)
    return command

##################################################################
##
## Helper funcs for conversion between internal and physical units
##
def getINTfromRPM(vrpm, pulse_div=0):
    vint=vrpm*r_fullstep*r_microstep*2**pulse_div*2048*32/(16*10**6*60)
    return int(vint)
def getRPMfromINT(vint, pulse_div=0):
    vrpm=vint*16*10**6*60/(r_fullstep*r_microstep*2**pulse_div*2048*32)
    return int(vrpm)

#####################
##
## Other helper funcs
##
def getSpeed(sd):
    if sd == 1:
        return LOW
    elif sd == 2:
        return MED
    elif sd == 3:
        return HIGH
    
def connect(dev_addr):
    connected = False
    curr_dev = 0 # ttyACM*/ttyUSB* dev number
    curr_dev_addr = ""
    while not connected:
        curr_dev_addr = dev_addr + str(curr_dev)
        try:
            logger.info("Trying device %s" % curr_dev_addr)
            device = serial.Serial(curr_dev_addr, BAUD, timeout=2)
            connected = True
            logger.info("Successfully opened port on device %s" %
                        curr_dev_addr)
        except (serial.SerialException, FileNotFoundError) as e:
            logger.info("Tried device %s and failed" % curr_dev_addr)
            curr_dev += 1
            curr_dev %= 10
            time.sleep(2)
    return device
    
######################################################
##
## Handle command strings from pipe and send to device
##
def handleCommand(cmd_string, device):
    cmd = cmd_string.strip().split("|")

    if cmd[0] == "S": # motor speed dial
        cmd_num = int(cmd[1])
        if cmd_num == 3: # shut off motors
            speed_int = 0
        else:
            speed = getSpeed(int(cmd[2]))
            speed_int = getINTfromRPM(speed)        
        flags = cmd[3]
        
        for motor in range(len(flags)):
            if int(flags[motor]) == 1:
                bin_cmd = getMotorBinCommand(ADDRESS, cmd_num, TYPENUM, motor, speed_int)
                device.write(bin_cmd)
                logger.info("Sent command to motor %d" % (motor+1))
                time.sleep(0.1) # Sleep for 100 ms as suggested by Trinamic datasheets
    elif cmd[0] == "B": # motor bridge mode
        try:
            bin_cmd = getMotorBinCommand(int(cmd[1]), int(cmd[2]), int(cmd[3]), int(cmd[4]), int(cmd[5]))
            device.write(bin_cmd)
            logger.info("Sent brige mode command to motor %d" % (int(cmd[4])+1))
        except:
            logger.error("Failed to send bridge mode command to device")

    else: # oxy commands
        try:
            cmd_num = int(cmd[0])
            rw=int(cmd[1])
            reg=int(cmd[2])
            data=int(cmd[3], base=2)
            bin_cmd = getOxyBinCommand(cmd_num, rw, reg, data)
            device.write(bytes(bin_cmd, encoding='utf-8'))
            logger.info("Sent command to OXY board")
        except:
            logger.error("Failed to send command to OXY board")
            
def main():
    global LOW, MED, HIGH

    # read stuff from config
    config = configparser.ConfigParser()
    config.read(CONF)    
    oxydevice = connect(config['devices']['oxy'])
    motordevice = connect(config['devices']['motors'])
    LOW = int(config['motor_speed']['low'])
    MED = int(config['motor_speed']['medium'])
    HIGH = int(config['motor_speed']['high'])

    logger.info("Connected to devices, now waiting for commands..")
    while True:
        with open("/srv/http/cgi-bin/dghandler.pipe", "r") as pipe:
            while True:
                cmd_string = pipe.readline()
                if len(cmd_string) == 0:
                    break
                logger.info("Received command string from pipe: %s" % cmd_string)
                try:
                    if cmd_string[0] == "B" or cmd_string[0] == "S":
                        handleCommand(cmd_string, motordevice)
                    else:
                        handleCommand(cmd_string, oxydevice)
                except:
                    break
                
main()
