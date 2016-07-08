#ifndef READCONF_H
#define READCONF_H

#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Type.h"

VOID read_conf(CHAR *port, CHAR *httpsport, CHAR *size, CHAR *processes, CHAR *listen, CHAR *events);

#endif
