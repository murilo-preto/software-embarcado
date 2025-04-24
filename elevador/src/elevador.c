#include "sinais.h"
#include "bsp.h"
#include <stdio.h>


void TElevador_display(int andar);
#define UM_SEG (QTimeEvtCtr)(BSP_TICKS_PER_SEC)

enum InternalSignals {
    TIMEOUT_SIG = MAX_SIG 
};


typedef struct TElevadorTag {
    QActive super;       
    int id;
    int andar_atual;     
    int andar_destino;   
    int status;          
    int porta_aberta;    
    QTimeEvt timeEvt;    
} TElevador;

static TElevador l_TElevador;

static QState TElevador_parado(TElevador * const me, QEvt const * const e);
static QState TElevador_movimento(TElevador * const me, QEvt const * const e);
static QState TElevador_inicial(TElevador * const me, QEvt const *e);
static QState TElevador_porta_aberta(TElevador * const me, QEvt const * const e);

QActive * const AO_tmicro = (QActive *)&l_TElevador;

void TElevador_actor() {
    TElevador * const me = &l_TElevador;
    QActive_ctor(&me->super, Q_STATE_CAST(&TElevador_inicial)); // Initialize the base class
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U); // Initialize the time event
}

/* Initial state of the elevator */
QState TElevador_inicial(TElevador * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    // Subscribe to relevant signals
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QActive_subscribe((QActive *)me, OPEN_SIG);   // Open door
    QActive_subscribe((QActive *)me, CLOSE_SIG);  // Close door
    QActive_subscribe((QActive *)me, SOBE_BOTAO_SIG); 

    // Arm the time event for periodic signals
    QTimeEvt_armX(&me->timeEvt, UM_SEG, 0);

    // Transition to the "parado" (stopped) state
    return Q_TRAN(&TElevador_parado);
}

/*..........................................................................*/
/* State: Elevator is stopped */
QState TElevador_parado(TElevador * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Sinal recebido em TElevador_parado: %d\n", e->sig);

    switch (e->sig) {
        case OPEN_SIG: { // Request to open door
            printf("Abrindo porta %d\n", id_elevador);
            BSP_porta(id_elevador);
            id_elevador = 0;
            break;
        }
        case SOBE_BOTAO_SIG: { // Request to close door
            printf("Acionando botão sobe %d\n", id_elevador);
            BSP_botao_sobe(id_elevador);
            id_elevador = 0;
            break;  
        }
        case CLOSE_SIG: { // Request to close door
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top); // Pass unhandled signals to the top state
            break;
        }
    }
    return status;
}

/*..........................................................................*/
/* State: Elevator is moving */
QState TElevador_movimento(TElevador * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Signal received in TElevador_movimento: %d\n", e->sig);

    switch (e->sig) {
        case OPEN_SIG: { // Request to open door (ignored while moving)
            status = Q_HANDLED(); // Ignore door open requests while moving
            break;
        }
        case CLOSE_SIG: { // Request to close door
            status = Q_HANDLED(); // Door already closed in movement state
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top); // Pass unhandled signals to the top state
            break;
        }
    }
    return status;
}

/*..........................................................................*/
/* State: Elevator door is open */
static QState TElevador_porta_aberta(TElevador * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Signal received in TElevador_porta_aberta: %d\n", e->sig);

    switch (e->sig) {
        case CLOSE_SIG: { // Request to close door
            BSP_porta(me->porta_aberta); // acionacarroA - close door
            me->porta_aberta = 0;
            status = Q_TRAN(&TElevador_parado); // Transition to "parado" state
            break;
        }
        default: {
            status = Q_SUPER(&QHsm_top); // Pass unhandled signals to the top state
            break;
        }
    }
    return status;
}