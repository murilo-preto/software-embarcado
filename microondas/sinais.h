/*
 * sinais.h
 *
 *  Created on: 05/08/2013
 *      Author: Amaury
 */

#ifndef SINAIS_H_
#define SINAIS_H_

 #include "qp_port.h"
enum ToastOvenSignals {
    OPEN_SIG = Q_USER_SIG,
    CLOSE_SIG,
    PLUS1_SIG,
    CANCEL_SIG,
    TIME_TICK_SIG,
    TERMINATE_SIG, /* terminate the application */
	MAX_SIG
};

#define MAX_PUB_SIG TERMINATE_SIG+1
#define QUEUESIZE ((uint8_t)5)
#define POOLSIZE ((uint8_t)5)

typedef struct MicroEvtTag {
/* protected: */
    QEvt super;
/* public: */
} MicroEvt;

extern QActive * const AO_tmicro;
extern void TMicro_ctor();

#endif /* SINAIS_H_ */
