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

// static TElevador l_TElevador;
static TElevador l_TElevador1, l_TElevador2, l_TElevador3;

QActive * const AO_tmicro1 = (QActive *)&l_TElevador1;
QActive * const AO_tmicro2 = (QActive *)&l_TElevador2;
QActive * const AO_tmicro3 = (QActive *)&l_TElevador3;

static QState TElevador_parado(TElevador * const me, QEvt const * const e);
static QState TElevador_movimento(TElevador * const me, QEvt const * const e);
static QState TElevador_inicial(TElevador * const me, QEvt const *e);
static QState TElevador_porta_aberta(TElevador * const me, QEvt const * const e);

// QActive * const AO_tmicro = (QActive *)&l_TElevador;

// void TElevador_actor() {
//     TElevador * const me = &l_TElevador;
//     QActive_ctor(&me->super, Q_STATE_CAST(&TElevador_inicial)); // Initialize the base class
//     QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U); // Initialize the time event
// }

void TElevador_actor(int id) {
    TElevador * me;
    switch(id) {
        case 1: me = &l_TElevador1; break;
        case 2: me = &l_TElevador2; break;
        case 3: me = &l_TElevador3; break;
        default: return;
    }
    me->id = id; // Defina o ID do elevador
    QActive_ctor(&me->super, Q_STATE_CAST(&TElevador_inicial));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U);
}

/* Initial state of the elevator */
QState TElevador_inicial(TElevador * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    // Subscribe to relevant signals
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QActive_subscribe((QActive *)me, OPEN_SIG);   // Open door
    QActive_subscribe((QActive *)me, CLOSE_SIG);  // Close door
    QActive_subscribe((QActive *)me, SOBE_BOTAO_SIG); 
    QActive_subscribe((QActive *)me, DESCE_BOTAO_SIG); 
    QActive_subscribe((QActive *)me, PORTA_ABRIU_SIG); 


    // Arm the time event for periodic signals
    QTimeEvt_armX(&me->timeEvt, UM_SEG, 0);

    // Transition to the "parado" (stopped) state
    return Q_TRAN(&TElevador_parado);
}

/*..........................................................................*/
/* State: Elevator is stopped */
QState TElevador_parado(TElevador * const me, QEvt const * const e) {
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
        case SOBE_BOTAO_SIG: { // Request to close door
            MicroEvt *evt = (MicroEvt *)e;
            if (evt->elevador_id == me->id) { // Verifique o ID
                printf("Acionando botao sobe %d\n", me->id);
                BSP_botao_sobe(me->id);
                BSP_porta(me->id, 1);
                status = Q_HANDLED();
            }
            break;  
        }
        case DESCE_BOTAO_SIG: { // Request to close door
            MicroEvt *evt = (MicroEvt *)e;
            if (evt->elevador_id == me->id) { // Verifique o ID
                printf("Acionando botao desce %d\n", me->id);
                BSP_botao_desce(me->id);
                BSP_porta(me->id, (-1));
                status = Q_HANDLED();
            }
            break;  
        }
        case CLOSE_SIG: { // Request to close door
            status = Q_HANDLED();
            break;
        }
        case PORTA_ABRIU_SIG: {
            MicroEvt *evt = (MicroEvt *)e;
            if (evt->elevador_id != me->id) { // Ignore if not for this elevator
                printf("Porta do elevador %d terminou de abrir\n", me->id);
                BSP_porta_abriu(me->id, 1);
                status = Q_TRAN(&TElevador_porta_aberta);
            }
            break;
        }
        default: {
            status = Q_HANDLED();
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
    // printf("RECEBIDO: <TElevador_movimento>: %d\n", e->sig);

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

    switch (e->sig) {
        case PORTA_ABRIU_SIG: {
            // Ignore or log the event to prevent reprocessing
            printf("PORTA_ABRIU_SIG ignored in TElevador_porta_aberta\n");
            status = Q_HANDLED();
            break;
        }
        default: {
            printf("Unhandled signal in TElevador_porta_aberta: %d\n", e->sig);
            status = Q_SUPER(&QHsm_top); // Pass unhandled signals to the top state
            break;
        }
    }
    return status;
}