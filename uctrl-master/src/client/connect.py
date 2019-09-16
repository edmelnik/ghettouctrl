import serial
import time

def connect(config, logger, baud):
    connected = False
    curr_dev = 0 # ttyACM*/ttyUSB* dev number
    dev_addr = config['input']['device']
    curr_dev_addr = ""
    while not connected:
        curr_dev_addr = dev_addr + str(curr_dev)
        try:
            logger.info("Trying device %s" % curr_dev_addr)
            device = serial.Serial(curr_dev_addr, baud, timeout=2)
            connected = True
            logger.info("Successfully opened port on device %s" %
                        curr_dev_addr)
        except (serial.SerialException, FileNotFoundError) as e:
            logger.info("Tried device %s and failed" % curr_dev_addr)
            curr_dev += 1
            curr_dev %= 10
            time.sleep(2)
    return device
