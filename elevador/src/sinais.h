#ifndef SINAIS_H_
#define SINAIS_H_
#include "qp_port.h"

enum SinaisElevador {
    OPEN_SIG = Q_USER_SIG,
    CLOSE_SIG,
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
extern void TElevador_actor();

#endif /* SINAIS_H_ */
