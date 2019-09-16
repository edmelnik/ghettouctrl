#include <SoftwareSerial.h>
#include <string.h>

#define BAUD 9600
#define MOTOR_BAUD 115200

const static unsigned int CMD_LEN PROGMEM = 9;
SoftwareSerial motors(4, 5);

struct cmd{
    unsigned char address, command, type;
    unsigned char motor, value[4], checksum;
};

size_t sendCommand(struct cmd _cmd){
    int i;
    size_t retval;
    unsigned char buffer[9];    
    buffer[0]=_cmd.address;
    buffer[1]=_cmd.command;
    buffer[2]=_cmd.type;
    buffer[3]=_cmd.motor;
    for(i=4; i<8; i++) buffer[i] = _cmd.value[i%4];    
    buffer[8]=_cmd.checksum;

    retval=motors.write(buffer, CMD_LEN);
    return retval;
}

int setChecksum(struct cmd *_cmd){
    int i, retval;
    unsigned char ck;
    if(_cmd->address <= 0 || _cmd->command <=0){
	retval = -1; 
    }
    else{
	ck=_cmd->address;
	ck+=_cmd->command;
	ck+=_cmd->type;
	ck+=_cmd->motor;
	for(i=0; i<4; i++) ck+=_cmd->value[i];    
	_cmd->checksum = ck;
	retval = 1;
    }
    return 1;
}

int makeCommand(struct cmd *_cmd, int addr, int cmd, int type, int motor, int value){
    // unsigned char addr, cmd, type, motor, value[4];
    int ret;
    unsigned char temp;
    
    ret = sprintf(temp, "%x", addr);
    _cmd->address = temp;    
    ret = sprintf(temp, "%x", cmd);
    _cmd->command = temp;    
    ret =sprintf(temp, "%x", type);
    _cmd->type = temp;
    sprintf(temp, "%x", motor);
    _cmd->motor = temp;
    // value how??
}

void setup(){
    Serial.begin(BAUD);
    motors.begin(MOTOR_BAUD);
}

int cur_iter = 0;
void loop(){
    char ret;
    struct cmd _cmd;
    makeCommand(&_cmd, 1, 1, 0, 0, 1000);
    // _cmd.address = 0x01;
    // _cmd.command = 0x01;
    // _cmd.type = 0x00;
    // _cmd.motor = 0x00;
    Serial.println(_cmd.address, HEX);
    _cmd.value[0] = 0x00;
    _cmd.value[1] = 0x00;
    _cmd.value[2] = 0x00;
    _cmd.value[3] = 0xc8;
    
    setChecksum(&_cmd);
    sendCommand(_cmd);
    
    delay(5000);
}
