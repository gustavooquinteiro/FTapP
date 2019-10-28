#ifndef _CLIENT_H
#define _CLIENT_H

#include "user_interface.h"
#include "transport.h"
#include <stdio.h>

int send_package(FILE * arquivo);
int main(int argc, char **argv);

#endif