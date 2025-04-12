/*
 * pisca.h
 *
 *  Created on: 14 de mar de 2017
 *      Author: tamandua32
 */

#ifndef PISCA_H_
#define PISCA_H_
#include "qp_port.h"

enum DPPSignals {
    B1_SIG = Q_USER_SIG, /* published by BSP */
    MAX_PUB_SIG,          /* the last published signal */

    B2_SIG,           /* posted direclty to Pisca from BSP */
    MAX_SIG               /* the last signal */
};


typedef struct PiscaEvtTag {
/* protected: */
    QEvt super;

/* public: */
    uint8_t i;
} PiscaEvt;


#define QUEUESIZE ((uint8_t)5)
#define POOLSIZE ((uint8_t)5)

void Pisca_ctor(void);

extern QActive * const AO_Pisca;

#endif /* PISCA_H_ */
