/*
 * bsp.h
 *
 *  Created on: 14 de mar de 2017
 *      Author: tamandua32
 */

#ifndef BSP_H_
#define BSP_H_

#define BSP_TICKS_PER_SEC   100U

#define BUFLEN  512  //Max length of buffer
#define PORTIN  8888   //The port on which to listen for incoming data
#define PORTOUT 8889

void bsp_init(int t);
void sendUDP(char *sig);

#endif /* BSP_H_ */