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
    int andar_atual;     
    int andar_destino;   
    int status;          
    int porta_aberta;    
    QTimeEvt timeEvt;    
} TElevador;

static TElevador l_TElevador;

static QState TElevador_inicial(TElevador * const me, QEvt const *e);
static QState TElevador_parado(TElevador * const me, QEvt const * const e);
static QState TElevador_movimento(TElevador * const me, QEvt const * const e);
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

    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QActive_subscribe((QActive *)me, OPEN_SIG);  
    QActive_subscribe((QActive *)me, CLOSE_SIG);
    QActive_subscribe((QActive *)me, SOBE_BOTAO_SIG); 
    QActive_subscribe((QActive *)me, DESCE_BOTAO_SIG); 
    QActive_subscribe((QActive *)me, PORTA_ABRIU_SIG); 
    QActive_subscribe((QActive *)me, CABINE_SIG); 
    QActive_subscribe((QActive *)me, ANDAR_SIG); 
    QActive_subscribe((QActive *)me, PARADO_SIG); 

    QTimeEvt_armX(&me->timeEvt, UM_SEG, 0);
    return Q_TRAN(&TElevador_parado);
}

/*..........................................................................*/
/* State: Elevator is stopped */
QState TElevador_parado(TElevador * const me, QEvt const * const e) {
    QState status = Q_HANDLED(); // Default to handled state

    switch (e->sig) {
        case OPEN_SIG: {
            printf("Abrindo porta %d\n", andar);
            status = Q_HANDLED();
            break;
        }
        case CABINE_SIG: {
            printf("Cabine acionada %d\n", andar);
            BSP_ir_para_andar(andar);
            append_fila(fila, andar);
            print_fila(fila);
            status = Q_HANDLED();   
            break;
        }
        case SOBE_BOTAO_SIG: { 
            printf("Acionando botao sobe %d\n", andar);
            status = Q_HANDLED();
            break;  
        }
        case DESCE_BOTAO_SIG: {
            printf("Acionando botao desce %d\n", andar);
            status = Q_HANDLED();
            break;  
        }
        case CLOSE_SIG: {
            status = Q_HANDLED();
            break;
        }
        case PORTA_ABRIU_SIG: {
            printf("Porta do elevador %d terminou de abrir\n", andar);
            status = Q_HANDLED();
            break;
        }
        case ANDAR_SIG: {
            printf("Elevador passou pelo andar %d\n", andar);
            BSP_atualiza_display(andar);
            status = Q_HANDLED();
            break;
        }
        case PARADO_SIG: {
            printf("Elevador parado no andar %d\n", andar);
            BSP_atualiza_display(andar);
            status = Q_HANDLED();
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