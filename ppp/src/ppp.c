#include "sinais.h"
#include "bsp.h"
#include <stdio.h>


void TElevador_display(int andar);
#define UM_SEG (QTimeEvtCtr)(BSP_TICKS_PER_SEC)

enum InternalSignals {
    TIMEOUT_SIG = MAX_SIG 
};


typedef struct PointTag {
    QActive super;       
    int id;
    QTimeEvt timeEvt;    
} Point;

static Point l_PointA, l_PointB;

QActive * const PointA = (QActive *)&l_PointA;
QActive * const PointB = (QActive *)&l_PointB;

static QState S_STARTING(Point * const me, QEvt const *e);
static QState S_LISTENING(Point * const me, QEvt const *e);
static QState S_REQ_SENT(Point * const me, QEvt const * const e);
static QState S_ACK_REC(Point * const me, QEvt const * const e);
static QState S_ACK_SENT(Point * const me, QEvt const * const e);
static QState S_OPENED(Point * const me, QEvt const * const e);

void print_hex_string(const char *hex_str) {
    for (size_t i = 0; hex_str[i] && hex_str[i+1]; i += 2) {
        unsigned int byte;
        sscanf(&hex_str[i], "%2x", &byte);
        printf("%02X ", byte);
    }
    printf("\n");
}

void Point_actor(int id) {
    Point * me;
    switch(id) {
        case 1: me = &l_PointA; break;
        case 2: me = &l_PointB; break;
        default: return;
    }
    me->id = id;
    QActive_ctor(&me->super, Q_STATE_CAST(&S_STARTING));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U);
}

/* Initial state of the point */
QState S_STARTING(Point * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    // Subscribe to relevant signals
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QActive_subscribe((QActive *)me, OPEN_SIG);  
    QActive_subscribe((QActive *)me, CLOSE_SIG); 
    QActive_subscribe((QActive *)me, DATA_SIG); 

    // Arm the time event for periodic signals
    QTimeEvt_armX(&me->timeEvt, UM_SEG, 0);
    return Q_TRAN(&S_LISTENING);
}

/*..........................................................................*/
/* State: Listening */
QState S_LISTENING(Point * const me, QEvt const * const e) {
    QState status = Q_HANDLED(); // Default to handled state
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            printf(" S_LISTENING: ENTRY_SIG \n");
            status = Q_HANDLED();
            break;
        }
        case OPEN_SIG: {
            printf(" S_LISTENING: OPEN_SIG \n");
            BSP_send_configure_request(PointB);
            status = Q_TRAN(S_REQ_SENT);
            break;
        }
        case ACK_RECEIVED_SIG: {
            printf(" S_LISTENING: ACK_RECEIVED_SIG \n");
            print_hex_string(((MicroEvt *)e)->data);
            BSP_decode_configure_request(((MicroEvt *)e)->data, ((MicroEvt *)e)->size);
            status = Q_TRAN(S_ACK_REC);
            break;
        }
        case CLOSE_SIG: {
            printf(" S_LISTENING: CLOSE_SIG \n");
            status = Q_HANDLED();
            break;
        }
        default: {
            printf(" S_LISTENING: Unhandled signal %d\n", e->sig);
            status = Q_SUPER(&QHsm_top); 
            break;
        }
    }
    return status;
}


/*..........................................................................*/
/* State: Elevator is stopped */
QState S_REQ_SENT(Point * const me, QEvt const * const e) {
    QState status = Q_HANDLED(); // Default to handled state

    switch (e->sig) {
        case OPEN_SIG: {
            MicroEvt *evt = (MicroEvt *)e;
            if (evt->elevador_id == me->id) { // Verifique o ID
                printf("Abrindo porta %d\n", me->id);
                BSP_porta(me->id, 0);
                status = Q_HANDLED();
            }
            break;
        }
        case CLOSE_SIG: { // Request to close door
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top); 
            break;
        }
    }
    return status;
}

/*..........................................................................*/
/* State: Elevator is moving */
QState S_ACK_REC(Point * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case OPEN_SIG: { 
            status = Q_HANDLED(); 
            break;
        }
        case CLOSE_SIG: { 
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top); 
            break;
        }
    }
    return status;
}

/*..........................................................................*/
/* State: Elevator door is open */
static QState S_ACK_SENT(Point * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        default: {
            printf("Unhandled signal in S_ACK_SENT: %d\n", e->sig);
            status = Q_SUPER(&QHsm_top); // Pass unhandled signals to the top state
            break;
        }
    }
    return status;
}