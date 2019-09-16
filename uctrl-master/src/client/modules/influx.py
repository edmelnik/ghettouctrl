##see client.py for input details but "values" is passed as a list from client.py
##this output device just gets that in raw for to the cloud
##[TIMESTAMP D1 D2 D3 ... Dn]

##download the influx db python client with "pip install influxdb"

##todo
## Decide if we want to hardcode these as needed, or configure. Probably configure, but at that point it's like what we have now and we basically write the whole shebang
##   might have to add influx config file path to config, or all this shit below. otherwise it'll be a big shit sandwhich of data
## rewrite this all in the form of serieshelper example (smh) https://influxdb-python.readthedocs.io/en/latest/examples.html

from influxdb import InfluxDBClient
import getoffset

###____   SPECIFIC PROJECT CONFIG   ____###
PROJECT_NAME = "o2_uctrl"
TAG_o2 = "4o2avg"
TAG_pressure = "pressure"

###____   INFLUX SERVER CONFIG   ____###
#Uncomment to use Sandbox Server config
HOST = '35.243.148.141'
PORT = 8086
USERNAME = ""
PASSWORD = ""
DATABASE = "influxout_vanilla"

###____   INFLUX SERVER CONFIG   ____###
#Uncomment to use Production InfluxDB Server config
#HOST = 'app.psnergy.com'
#PORT = 8086
#USERNAME = "influxout_vanilla"
#PASSWORD = "emacssucks"
#DATABASE = influxout_vanilla

###____   INFLUX SERVER CONFIG   ____###
#Uncomment to use Local InfluxDB instance config
#HOST = 'localhost'
#PORT = 8086
#USERNAME = ""
#PASSWORD = ""
#DATABASE = influxout_vanilla

def buildClient():
    client = InfluxDBClient(host=HOST, port=PORT, username = USERNAME, password = PASSWORD)
    return(client)

client = buildClient()
client.switch_database(DATABASE)

# This list would be useful in the future to detect and tag ERR, STS and CAL messages
nanlist = []
for num in range(0, 9):
    nanlist.append("ERR"+str(num))
for num in range(0,5):
    nanlist.append("STS"+str(num))
for num in range(0,3):
    nanlist.append("CAL"+str(num))
nanlist.append("CAL")

curr_prs=[0.0, 0.0, 0.0, 0.0]

def getDict(values):
    dict_values = {"measurement": PROJECT_NAME, "tags": {}} # o2 values
    dict_values["fields"] = {}    
        
    if values[1] == "OXY":
        tag = TAG_o2
        dict_values['tags']['label'] = tag        
        
        for i in range(2, len(values)):
            if values[i].isdigit():
                value = getoffset.getOffset(float(values[i])*0.1, curr_prs[i-2], i-2)
                value = round(value, 3)
                sid = "d"+str(i-1)
                dict_values['fields'][sid] = value
            else:
                if values[i][:3] == "CAL": # Calibration status
                    try: calsts = int(values[i][3])
                    except IndexError: continue
                    sid = "calsts"+str(i-1)
                    dict_values['fields'][sid] = calsts
                elif values[i][:3] == "STS": # O2 status
                    try: sts = int(values[i][3])
                    except IndexError: continue
                    sid = "sts"+str(i-1)
                    dict_values['fields'][sid] = sts            
    elif values[1] == "PRS":
        tag = TAG_pressure
        dict_values['tags']['label'] = tag
        for i in range(2, len(values)):
            try: float(values[i])
            except ValueError: continue
            curr_prs[i-2] = float(values[i])*68.94757
            sid = "d"+str(i-1)
            value = float(values[i])
            dict_values['fields'][sid] = value
    else:
        return -1
    
    dict_list = [dict_values]
    return(dict_list)

def sendData(values):
    dict_list = getDict(values)
    if dict_list != -1:
        # print(dict_list)
        client.write_points(dict_list, batch_size=5)        
    return 1

