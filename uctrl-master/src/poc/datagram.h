#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct cmd{
    unsigned int command, reg, data;
    char checksum, rw;
};

int parseCommand(struct cmd *_cmd, char cmd[13]);
char getChecksum(char cmd[13]);
