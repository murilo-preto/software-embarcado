#include "qp_port.h"

#ifndef SINAIS_H_
#define SINAIS_H_
#define MAX_PUB_SIG TERMINATE_SIG+1
#define QUEUESIZE ((uint8_t)10)
#define POOLSIZE ((uint8_t)5)

enum SinaisElevador {
    OPEN_SIG = Q_USER_SIG,
    CLOSE_SIG,
    DATA_SIG,
    MESSAGE_SIG,
    ACK_RECEIVED_SIG,
    TIME_TICK_SIG,
    TERMINATE_SIG,
	MAX_SIG
};

typedef struct MicroEvtTag {
    QEvt super;
    uint8_t elevador_id;  // Adicione o ID do elevador ao evento
    char data[512];
    int size;
} MicroEvt;

typedef struct {
    int code;
    int id;
    int length;
    int MRU;
    int ACCM;
    int auth_prot;
    int quality_prot;
    int magic_number;
    int protocol_field_comp;
    int addr_control_comp;
} PPP_Configuration;

extern QActive * const PointA;
extern QActive * const PointB;
extern QActive * const AO_tmicro;
extern void Point_actor();

#endif /* SINAIS_H_ */
