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
    int queue;
    int andar_atual;
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


static QState append_and_call(TElevador * const me, uint8_t fila[], uint8_t andar, int direc) {
    append_fila(fila, andar, direc);
    if (me->queue == 0) {
        me->queue = 1;
        return Q_TRAN(&TElevador_movimento);
    } else {
        return Q_HANDLED();
    }
}


/* Initial state of the elevator */
QState TElevador_inicial(TElevador * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    me->queue = 0;
    me->andar_atual = 1;

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
        case TIME_TICK_SIG: {
            printf("Parado: Tick\n");
            status = Q_HANDLED();
            break;
        }
        case PARADO_SIG: {
            printf("Parado: Elevador parado no andar %d\n", andar);
            BSP_luz_botao_cabine(andar, -1);
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
            BSP_luz_botao_cabine(andar, 1);
            status = append_and_call(me, fila, andar, 0); // Add the current floor to the queue
            break;
        }
        case SOBE_BOTAO_SIG: { 
            printf("Parado: Acionando botao sobe %d\n", andar);
            BSP_liga_botao_andar(andar, 1);
            status = append_and_call(me, fila, andar, 1);
            break;  
        }
        case DESCE_BOTAO_SIG: {
            printf("Parado: Acionando botao desce %d\n", andar);
            BSP_liga_botao_andar(andar, -1);
            status = append_and_call(me, fila, andar, -1);
            break;  
        }
        case CLOSE_SIG: {
            status = Q_HANDLED();
            break;
        }
        case PORTA_ABRIU_SIG: {
            printf("Parado: Porta do elevador %d terminou de abrir\n", andar);
            
            me->andar_atual = andar;
            printf("Parado -> Espera\n");
            status = Q_TRAN(&TElevador_espera);
            break;
        }
        case ANDAR_SIG: {
            int direc = direcao_fila(fila, me->andar_atual);
            printf("Parado: Elevador passou pelo andar %d, com sentido %d\n", andar, direc);

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
            BSP_porta(me->andar_atual, +1);
            status = Q_HANDLED();
            break;
        }
        case PORTA_FECHOU_SIG: {
            printf("Movimento: Porta %d fechou\n", andar);

            if (fila[0] != 0){ // Fila nao vazia
                if (fila[0] != me->andar_atual) { // Andar destino diferente do atual
                    BSP_ir_para_andar(fila[0]); // Vai para proximo andar da lista
                    status = Q_HANDLED();
                }
                else { // Andar destino igual ao atual
                    printf("Movimento -> Parado\n");
                    BSP_atualiza_display(andar);
                    BSP_luz_botao_cabine(andar, -1);
                    BSP_porta(andar, -1);

                    me->andar_atual = fila[0];
                    atualiza_fila(fila);

                    status = Q_TRAN(&TElevador_parado);
                }
            }
            else {
                me->queue = 0;
                status = Q_TRAN(&TElevador_parado); // No more floors in the queue
            }
            
            break;
        }
        case ANDAR_SIG: {
            int direc = direcao_fila(fila, me->andar_atual);
            printf("Movimento: Elevador passou pelo andar %d, com sentido %d\n", andar, direc);

            BSP_atualiza_display(andar);
            status = Q_HANDLED();
            break;
        }
        case CABINE_SIG: {
            printf("Movimento: Cabine acionada %d\n", andar);
            BSP_luz_botao_cabine(andar, 1);
            status = append_and_call(me, fila, andar, 0); // Add the current floor to the queue
            break;
        }
        case PARADO_SIG: {
            printf("Movimento -> Parado\n");
            BSP_atualiza_display(andar);
            BSP_porta(andar, -1);
            BSP_luz_botao_cabine(andar, -1);
            
            me->andar_atual = fila[0];
            atualiza_fila(fila);

            BSP_desliga_botao_andar(fila, andar);

            status = Q_TRAN(&TElevador_parado);
            break;    
        }
        case OPEN_SIG: {
            status = Q_HANDLED();
            break;
        }
        case CLOSE_SIG: {
            status = Q_HANDLED();
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
        case SOBE_BOTAO_SIG: { 
            printf("Movimento: Acionando botao sobe %d\n", andar);
            BSP_liga_botao_andar(andar, 1);
            status = append_and_call(me, fila, andar, 1);
            break;  
        }
        case DESCE_BOTAO_SIG: {
            printf("Movimento: Acionando botao desce %d\n", andar);
            BSP_liga_botao_andar(andar, -1);
            status = append_and_call(me, fila, andar, -1);
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
            QTimeEvt_armX(&me->timeEvt, 3 * UM_SEG, 0); // 3 segundos, sem repetição
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
        case CABINE_SIG : {
            printf("Espera: Cabine acionada %d\n", andar);
            BSP_luz_botao_cabine(andar, 1);
            status = append_and_call(me, fila, andar, 0);
            break;
        }
        case SOBE_BOTAO_SIG: { 
            printf("Espera: Acionando botao sobe %d\n", andar);
            BSP_liga_botao_andar(andar, 1);
            status = append_and_call(me, fila, andar, 1);
            break;  
        }
        case DESCE_BOTAO_SIG: {
            printf("Espera: Acionando botao desce %d\n", andar);
            BSP_liga_botao_andar(andar, -1);
            status = append_and_call(me, fila, andar, -1);
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
