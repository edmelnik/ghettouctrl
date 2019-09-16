#!/bin/python

import concurrent.futures
import sched
import time
import requests
import configparser
import sys
import logging
from systemd import journal
from apscheduler.schedulers.blocking import BlockingScheduler
from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.cron import CronTrigger

CONF = 'conf/periodic.conf'
logger = logging.getLogger('periodic_logger')
logger.addHandler(journal.JournalHandler())
logger.setLevel(logging.INFO)

###############################################
#
# Main routine class to handle command groups
#
###############################################
class Routine:
    def __init__(self, config, routine_id, desc, num_cmd, cmd_list=[], crontab_list=[]):
        self.main_conf=config
        self.num_cmd=num_cmd
        self.routine_id=routine_id
        self.cmd_list, self.crontab_list=cmd_list, crontab_list
        self.sched=BackgroundScheduler()
        for i in range(len(cmd_list)):
            self.sched.add_job(self.sendCommand,
                               CronTrigger.from_crontab(crontab_list[i]),
                               kwargs={'cmd':cmd_list[i]})
            
    def sendCommand(self, cmd):
        logger.info("Routine #"+str(self.routine_id+1)+": sending command "+cmd)
        address = "http://localhost/cgi-bin/picmd"
        try: r = requests.post(address, data={'code': cmd})
        except: return -1
        finally: return 1
        
    def run(self):
        global routine_status
        self.sched.start()
        status=1
        while True:
            if routine_status[self.routine_id]==0 and status==1:
                self.sched.pause()
                logger.info("Routine #" + str(self.routine_id+1)+" paused")
                status=0
            elif routine_status[self.routine_id]==1 and status==0:
                self.sched.resume()
                logger.info("Routine #" + str(self.routine_id+1)+" resumed")
                status=1
            time.sleep(10)
            
scheds=[]
routine_confs=[]
routines=[]
routine_status=[]
def main():
    logger.info("Started periodic command scheduler service")
    
    config = configparser.ConfigParser()
    config.read(CONF)
    global routine_status
    routine_status=[0 for x in range(len(config['routines']))]
    
    ######## Get all routine conf instances into a list
    i=0
    postfix='.routine'
    for routine in config['routines']:
        routine_name=routine+postfix
        temp_config = configparser.ConfigParser()
        try: temp_config.read('routines/'+routine_name)
        except:
            logger.error("Routine file named %s not found"%routine_name)
            continue
        routine_confs.append(temp_config)
        routine_status[i]=int(config['routines'][routine])
        i+=1
    
    ######## Get all routines to an Routine class instance list
    i=0
    for routine in routine_confs:
        cmd_list=[]
        crontab_list=[]
        num_cmd= len(list(routine.keys())[2:]) # First skipped since it's meta
        desc=routine['grp']['desc']
        for cmd_id in list(routine.keys())[2:]:
            cmd, crontab=routine[cmd_id]['cmd'], routine[cmd_id]['crontab']
            cmd_list.append(cmd); crontab_list.append(crontab)
        routine_instance=Routine(config, i, desc, num_cmd, cmd_list, crontab_list)
        routines.append(routine_instance)
        i+=1
    
    num_routines = len(routines)
    logger.info("Found %d routines to be executed periodically" % num_routines)
    
    executor = concurrent.futures.ThreadPoolExecutor(max_workers=num_routines)
    fs = []
    for routine in routines:
        future = executor.submit(routine.run)
        logger.info(future)
        fs.append(future)

    while True:
        try:
            i=0
            config.read(CONF)
            for routine in config['routines']:
                if config['routines'][routine]=="0":
                    routine_status[i]=0
                elif config['routines'][routine]=="1":
                    routine_status[i]=1
                i+=1
            time.sleep(10)
        except KeyboardInterrupt:
            sys.exit()

    try: concurrent.futures.wait(fs)
    except KeyboardInterrupt: sys.exit()
    
main()

