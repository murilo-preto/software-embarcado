#ifndef SINAIS_H_
#define SINAIS_H_
#include "qp_port.h"

enum SinaisElevador {
    OPEN_SIG = Q_USER_SIG,
    CLOSE_SIG,
    SOBE_BOTAO_SIG,
    DESCE_BOTAO_SIG,
    PORTA_ABRIU_SIG,
    TIME_TICK_SIG,
    TERMINATE_SIG,
	MAX_SIG
};

#define MAX_PUB_SIG TERMINATE_SIG+1

// #define QUEUESIZE ((uint8_t)5)
#define QUEUESIZE ((uint8_t)10)

#define POOLSIZE ((uint8_t)5)

typedef struct MicroEvtTag {
    QEvt super;
    uint8_t elevador_id;  // Adicione o ID do elevador ao evento
} MicroEvt;

extern QActive * const AO_tmicro1;
extern QActive * const AO_tmicro2;
extern QActive * const AO_tmicro3;

// typedef struct MicroEvtTag {
// /* protected: */
//     QEvt super;
// /* public: */
// } MicroEvt;

// extern uint8_t id_elevador;
// extern uint8_t pos_elevador1;
// extern uint8_t pos_elevador2;
// extern uint8_t pos_elevador3;

extern QActive * const AO_tmicro;
extern void TElevador_actor();

#endif /* SINAIS_H_ */
