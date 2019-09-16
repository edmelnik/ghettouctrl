#!/bin/python

'''
Client reads serial output on serial ports

Expected data format is a bytestring with an identifier tag, followed by a string individual sensor values, all seperated by whitespaces. Each datapoint is seperated by a newline (\n) and return carriage (\r) . Ex. (for n data points) input will be: 

b'TAG D1 D2 D3 ... Dn\n\r'

For each sensor Di

Each datapoint is output by client as a list of strings:

[TIMESTAMP TAG D1 D2 D3 ... Dn]

Configuration of n is done on the microcontroller. Check docs for special condition outputs

'''

'''
TODO Add reset condition if 3 or more timeouts detected
  - Number of timeouts to reset could be a config option
'''

'''
NOTES
- Note: client allows module queues to have a maximum of QMAXSIZE datapoints. At every CHK_DELAY, if the queue size is equal to QMAXSIZE, the queue is cleared (and buffered data lost)
'''

import serial
import time
import sys
import configparser
import importlib
import threading
import queue
import logging
import connect
from systemd import journal

BAUD = 9600

OUTPUT_FUNC = 'sendData'
CLIENT_CONF = 'conf/client.conf'
QMAXSIZE = 1000
CHK_DELAY = 100
queue_dict = {}
module_list = []

logger = logging.getLogger('client_logger')
logger.addHandler(journal.JournalHandler())
logger.setLevel(logging.INFO)

def initData(device):
    epoch = time.time()
    curr_time = time.time()
    while curr_time - epoch <= 2:
        getData(device)
        curr_time = time.time()
      
def getData(device):
    vals = []
    try:
        line = device.readline()
        curr_time = time.time()
        vals = line.decode('utf-8').rsplit()
        curr_time = str(curr_time)
        curr_time = curr_time[:curr_time.find('.')+3] # truncate to 2 decimal points
        vals.insert(0, curr_time)
        return vals;
    except (serial.SerialTimeoutException, UnicodeDecodeError) as e:
        logger.warning("Could not get data from serial device, probably a timeout")
        return vals
    except:
        return vals

def handleTimeout(device):
    logger.warning("Serial device timed out, resetting serial connection")
    device.close()
    device.open()
    return device

def doOutput(config, values):
    output = config['output']
    for module in output:
        if output[module] == "1":
            try:
                queue_dict[module].put(values)
            except:
                logger.warning("client.conf wants client to send data to nonexisting or misconfigured client")

class ModuleHandler():
    def __init__(self, module_name, q):
        self.module_queue = q
        self.module_name = module_name
    def handleOutput(self):
        while True:
            values = self.module_queue.get(block=True)
            if(values[0] == "END"):
                break
            try:
                status = getattr(sys.modules[self.module_name],
                                 OUTPUT_FUNC)(values)
            except:
                logger.error("Failed to push to queue for module %s: either it does not exist or its queue is full" % self.module_name)
                continue
        return 1;

output_threads = []

def main():
    config = configparser.ConfigParser()
    config.read(CLIENT_CONF)
    device = connect.connect(config, logger, BAUD)
    sys.path.append('modules/')
    for module in config['output']:
        try:        
            importlib.import_module(module)
            q = queue.Queue(QMAXSIZE)
            queue_dict[module] = q
            handler_instance = ModuleHandler(module, q)            
            t = threading.Thread(target=handler_instance.handleOutput, name="t_"+module)
            output_threads.append(t)
            t.start()
            module_list.append(module)
        except:
            logger.error("Module named %s not found, skipping its import and moving on"
                         % module)
            continue
        
    initData(device)
    counter=0
    timeout_counter=0
    while True:            
        config.read(CLIENT_CONF) # Read config for changes
        values = getData(device)
        if len(values) <= 1:
            timeout_counter+=1
            if timeout_counter>=10:
                device = handleTimeout(device)
        else:
            doOutput(config, values)
            timeout_counter=0
        counter+=1
        if counter > CHK_DELAY:
            for module in module_list:
                if queue_dict[module].full() == True:
                    logger.critical("Queue size for %s is full, clearing queue" % module) 
                    queue_dict[module].queue.clear()
            counter=0

def end():
    endval = ["END"]
    config = configparser.ConfigParser()
    config.read(CLIENT_CONF)
    output = config['output']
    for module in output:
        try:
            queue_dict[module].put(endval)
        except KeyError:
            continue
    for thread in output_threads:
        thread.join()

# Don't stop even if device gets disconnected
while True:
    try:
        main()
    except KeyboardInterrupt:
        logger.info("Got keyboard interrupt, closing client")
        end()
        sys.exit()
    except:
        logger.exception("Unknown error: see following traceback")
        end()
        continue
