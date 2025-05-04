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
    QTimeEvt timeEvt;    
} TElevador;

static TElevador l_TElevador;

static QState TElevador_inicial(TElevador * const me, QEvt const *e);
static QState TElevador_parado(TElevador * const me, QEvt const * const e);
static QState TElevador_movimento(TElevador * const me, QEvt const * const e);
static QState TElevador_espera(TElevador * const me, QEvt const * const e);

QActive * const AO_tmicro = (QActive *)&l_TElevador;

void TElevador_actor() {
    TElevador * const me = &l_TElevador;
    QActive_ctor(&me->super, Q_STATE_CAST(&TElevador_inicial)); // Initialize the base class
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U); // Initialize the time event
}

/* Initial state of the elevator */
QState TElevador_inicial(TElevador * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    QActive_subscribe((QActive *)me, OPEN_SIG);  
    QActive_subscribe((QActive *)me, CLOSE_SIG);

    QActive_subscribe((QActive *)me, SOBE_BOTAO_SIG); 
    QActive_subscribe((QActive *)me, DESCE_BOTAO_SIG); 

    QActive_subscribe((QActive *)me, PORTA_ABRIU_SIG); 
    QActive_subscribe((QActive *)me, PORTA_FECHOU_SIG); 

    QActive_subscribe((QActive *)me, CABINE_SIG); 

    QActive_subscribe((QActive *)me, ANDAR_SIG); 
    QActive_subscribe((QActive *)me, PARADO_SIG); 
    
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QTimeEvt_armX(&me->timeEvt, UM_SEG, 0);
    return Q_TRAN(&TElevador_parado);
}

/*..........................................................................*/
QState TElevador_parado(TElevador * const me, QEvt const * const e) {
    QState status = Q_HANDLED();

    switch (e->sig) {
        case Q_INIT_SIG: {
            printf("Parado: Init\n");
            status = Q_HANDLED();
            break;
        }
        case Q_ENTRY_SIG: {
            printf("Parado: Entry, andar %d\n", andar);
            status = Q_HANDLED();
            break;
        }
        case PARADO_SIG: {
            printf("Parado: Elevador parado no andar %d\n", andar);
            status = Q_HANDLED();
            break;
        }
        case OPEN_SIG: {
            printf("Parado: Abrindo porta %d\n", andar);
            status = Q_HANDLED();
            break;
        }
        case CABINE_SIG: {
            printf("Parado: Cabine acionada %d\n", andar);

            append_fila(fila, andar);
            print_fila(fila);

            printf("Parado -> Movimento\n");
            status = Q_TRAN(&TElevador_movimento);   
            break;
        }
        case SOBE_BOTAO_SIG: { 
            printf("Parado: Acionando botao sobe %d\n", andar);
            status = Q_HANDLED();
            break;  
        }
        case DESCE_BOTAO_SIG: {
            printf("Parado: Acionando botao desce %d\n", andar);
            status = Q_HANDLED();
            break;  
        }
        case CLOSE_SIG: {
            status = Q_HANDLED();
            break;
        }
        case PORTA_ABRIU_SIG: {
            printf("Parado: Porta do elevador %d terminou de abrir\n", andar);

            QTimeEvt_armX(&me->timeEvt, 3 * UM_SEG, 0); // 3 segundos, sem repetição
            printf("Parado -> Espera\n");
            
            status = Q_TRAN(&TElevador_espera);
            break;
        }
        case ANDAR_SIG: {
            printf("Parado: Elevador passou pelo andar %d\n", andar);
            BSP_atualiza_display(andar);
            status = Q_HANDLED();
            break;
        }
        case TIMEOUT_SIG: {
            printf("Parado: Elevador %d timeout\n", andar);
            break;
        }
        case Q_EXIT_SIG: {
            printf("Parado: Exit signal %d\n", andar);
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
QState TElevador_movimento(TElevador * const me, QEvt const * const e) {
    QState status = Q_HANDLED();
    
    switch (e->sig) {
        case Q_INIT_SIG: {
            printf("Movimento: Init\n");
            status = Q_HANDLED();
            break;
        }
        case Q_ENTRY_SIG: {
            printf("Entry: Movimento\n");
            BSP_porta(andar, +1); // Close the door while moving
            status = Q_HANDLED();
            break;
        }
        case PORTA_FECHOU_SIG: {
            printf("Movimento: Porta %d fechou\n", andar);

            if (fila[0] != 0){
                BSP_ir_para_andar(fila[0]); // Move to the next floor
                status = Q_HANDLED();
            }
            else {
                status = Q_TRAN(&TElevador_parado); // No more floors in the queue
            }
            
            break;
        }
        case ANDAR_SIG: {
            printf("Movimento: Elevador passou pelo andar %d\n", andar);
            BSP_atualiza_display(andar);
            status = Q_HANDLED();
            break;
        }
        case PARADO_SIG: {
            printf("Movimento -> Parado\n");
            BSP_atualiza_display(andar);
            BSP_porta(andar, -1);
            atualiza_fila(fila);
            print_fila(fila);
            status = Q_TRAN(&TElevador_parado);
            break;    
        }
        case OPEN_SIG: { // Request to open door (ignored while moving)
            status = Q_HANDLED(); // Ignore door open requests while moving
            break;
        }
        case CLOSE_SIG: { // Request to close door
            status = Q_HANDLED(); // Door already closed in movement state
            break;
        }
        case TIMEOUT_SIG: {
            printf("Movimento: Timeout\n");
            status = Q_HANDLED();
            break;
        }
        case TIME_TICK_SIG: {
            printf("Movimento: Tick\n");
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            printf("Movimento: Exit signal\n");
            status = Q_HANDLED();
            break;
        }
        default: {
            printf("Movimento: Evento nao tratado: %d\n", e->sig);
            status = Q_SUPER(&QHsm_top); // Pass unhandled signals to the top state
            break;
        }
    }
    return status;
}

/*..........................................................................*/
static QState TElevador_espera(TElevador * const me, QEvt const * const e) {
    QState status = Q_HANDLED();

    switch (e->sig) {
        case Q_INIT_SIG: {
            printf("Espera: Init\n");
            status = Q_HANDLED();
            break;
        }
        case Q_ENTRY_SIG: {
            printf("Espera: Entry\n");
            status = Q_HANDLED();
            break;
        }
        case TIMEOUT_SIG: {
            status = Q_HANDLED();
            break;
        }
        case TIME_TICK_SIG: {
            printf("Espera -> Movimento\n");
            status = Q_TRAN(&TElevador_movimento);
            break;
        }
        case Q_EXIT_SIG: {
            printf("Espera: Exit signal\n");
            status = Q_HANDLED();
            break;
        }
        default: {
            printf("Espera: Evento nao tratado: %d\n", e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}