/*
 ============================================================================
 Name        : IOTeste.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"

int main(int argc, char *argv[]) {
	char str[5000];
    bsp_init(0);
    while(1){
    	gets(str);
    	if(strncmp(str, "fim", 3) == 0)
    		exit(0);
    	sendUDP(str);
    }
}
