/*
* bsp.c
*
*  Created on: 14 de mar de 2017
*      Author: tamandua32
*/

#include "bsp.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

pthread_t tudpServer;

struct sockaddr_in si_other;
int s;
WSADATA wsaData;
LPHOSTENT lpHostEntry;

void sendUDP(char *sig) {
    int slen = sizeof(si_other);
    int siglen;

    if (s != -1) {
        siglen = strlen(sig);
        sendto(s, sig, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}

void *udpServer() {
    struct sockaddr_in si_other2;
    struct sockaddr_in si_me;

    unsigned int slen = sizeof(si_other), recv_len;
    char buf[BUFLEN];
    int ss = -1;

    ss = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ss != -1) {
        memset((char *)&si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(PORTIN);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(ss, &si_me, sizeof(si_me)) != -1) {
            while (1) {
                if ((recv_len = recvfrom(ss, buf, BUFLEN, 0, (struct sockaddr *)&si_other2, &slen)) != -1) {
                    buf[recv_len] = '\0';
                    printf("RECEBIDO: %s\n", buf);
                    fflush(stdout);
                }
            }
        }
    }
    return NULL;
}

void bsp_init(int t) {
    WSAStartup(MAKEWORD(2, 1), &wsaData);
    lpHostEntry = gethostbyname("127.0.0.1");
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORTOUT);
    si_other.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
    pthread_create(&tudpServer, NULL, udpServer, NULL);
}


void Q_onError(uint_fast8_t const error) {
    // Tratamento de erro
    // Pode ser usado para registrar o erro ou reiniciar o sistema
}

void QF_onStartup(void) {
    // Inicializações específicas do sistema, como temporizadores
}

void QF_onCleanup(void) {
    // Limpeza específica do sistema
}

void QF_onClockTick(void) {
    // Rotina de tratamento do clock tick
    // Pode ser usada para gerar eventos de tempo
}