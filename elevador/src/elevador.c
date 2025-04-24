#include "sinais.h"
#include "bsp.h"
#include <stdio.h>


void TElevador_display(int andar);

#define UM_SEG (QTimeEvtCtr)(BSP_TICKS_PER_SEC)

enum InternalSignals {
    TIMEOUT_SIG = MAX_SIG 
};

#define ANDAR_TERREO 0
#define ANDAR_MAXIMO 9
#define ELEVADOR_PARADO 0
#define ELEVADOR_SUBINDO 1
#define ELEVADOR_DESCENDO 2

typedef struct TElevadorTag {
    QActive super;       
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

void TMicro_ctor() {
    TElevador * const me = &l_TElevador;
    QActive_ctor(&me->super, Q_STATE_CAST(&TElevador_inicial)); // Initialize the base class
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U); // Initialize the time event
}

/* Initial state of the elevator */
QState TElevador_inicial(TElevador * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    // Initialize hardware and variables
    me->andar_atual = ANDAR_TERREO;   // Start at ground floor
    me->andar_destino = ANDAR_TERREO; // No destination initially
    me->status = ELEVADOR_PARADO;     // Initially stopped
    me->porta_aberta = 0;             // Door initially closed
    
    // Initialize display and elevator components
    sendUDP(0);  // acionaportaAS - door control
    TElevador_display(me->andar_atual); // Display current floor
    
    // Subscribe to relevant signals
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QActive_subscribe((QActive *)me, PLUS1_SIG);  // Used for floor selection
    QActive_subscribe((QActive *)me, OPEN_SIG);   // Open door
    QActive_subscribe((QActive *)me, CLOSE_SIG);  // Close door
    QActive_subscribe((QActive *)me, CANCEL_SIG); // Emergency stop

    // Arm the time event for periodic signals
    QTimeEvt_armX(&me->timeEvt, UM_SEG, UM_SEG);

    // Transition to the "parado" (stopped) state
    return Q_TRAN(&TElevador_parado);
}

/*..........................................................................*/
/* State: Elevator is stopped */
QState TElevador_parado(TElevador * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Signal received in TElevador_parado: %d\n", e->sig);

    switch (e->sig) {
        case OPEN_SIG: { // Request to open door
            if (!me->porta_aberta) {
                sendUDP(0); // acionaportaAS - open door
                me->porta_aberta = 1;
                status = Q_TRAN(&TElevador_porta_aberta); // Transition to "door open" state
            } else {
                status = Q_HANDLED(); // Door already open
            }
            break;
        }
        case CLOSE_SIG: { // Request to close door
            status = Q_HANDLED(); // Door already closed in parado state
            break;
        }
        case PLUS1_SIG: { // Floor selection (using PLUS1 signal as generic input)
            // For simplicity, this increments the target floor
            // In a real system, you'd use signal parameters to set the target floor
            me->andar_destino = (me->andar_atual + 1) % (ANDAR_MAXIMO + 1);
            
            if (me->andar_destino > me->andar_atual) {
                me->status = ELEVADOR_SUBINDO;
                sendUDP(2); // elevadorsobeonA - elevator going up
                sendUDP(6); // elevadorcabineonA - elevator cabin light on
                status = Q_TRAN(&TElevador_movimento); // Transition to "movimento" state
            } else if (me->andar_destino < me->andar_atual) {
                me->status = ELEVADOR_DESCENDO;
                sendUDP(4); // elevadordesceonA - elevator going down
                sendUDP(6); // elevadorcabineonA - elevator cabin light on
                status = Q_TRAN(&TElevador_movimento); // Transition to "movimento" state
            } else {
                status = Q_HANDLED(); // Already at the target floor
            }
            break;
        }
        case CANCEL_SIG: { // Emergency stop or reset
            me->andar_destino = me->andar_atual; // Cancel any destination
            status = Q_HANDLED(); // No state transition
            break;
        }
        case TIME_TICK_SIG: { // Timer tick
            TElevador_display(me->andar_atual); // Update display
            status = Q_HANDLED(); // No state transition
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
        case CANCEL_SIG: { // Emergency stop
            if (me->status == ELEVADOR_SUBINDO) {
                sendUDP(3); // elevadorsobeoffA - turn off up movement
            } else if (me->status == ELEVADOR_DESCENDO) {
                sendUDP(5); // elevadordesceoffA - turn off down movement
            }
            sendUDP(7); // elevadorcabineoffA - turn off cabin light
            
            me->status = ELEVADOR_PARADO;
            me->andar_destino = me->andar_atual; // Set destination to current floor
            status = Q_TRAN(&TElevador_parado); // Transition to "parado" state
            break;
        }
        case TIME_TICK_SIG: { // Timer tick
            // Simulate elevator movement (in a real system, you'd receive position feedback)
            if (me->status == ELEVADOR_SUBINDO && me->andar_atual < me->andar_destino) {
                // Slow movement simulation - move every 3 seconds
                static int tick_count = 0;
                tick_count++;
                if (tick_count >= 3) {
                    me->andar_atual++;
                    tick_count = 0;
                    TElevador_display(me->andar_atual);
                }
            } else if (me->status == ELEVADOR_DESCENDO && me->andar_atual > me->andar_destino) {
                // Slow movement simulation - move every 3 seconds
                static int tick_count = 0;
                tick_count++;
                if (tick_count >= 3) {
                    me->andar_atual--;
                    tick_count = 0;
                    TElevador_display(me->andar_atual);
                }
            }
            
            // Check if the elevator has reached its destination
            if (me->andar_atual == me->andar_destino) {
                // Stop the elevator
                if (me->status == ELEVADOR_SUBINDO) {
                    sendUDP(3); // elevadorsobeoffA - turn off up movement
                } else if (me->status == ELEVADOR_DESCENDO) {
                    sendUDP(5); // elevadordesceoffA - turn off down movement
                }
                sendUDP(7); // elevadorcabineoffA - turn off cabin light
                
                me->status = ELEVADOR_PARADO;
                
                // Open the door automatically upon arrival
                sendUDP(0); // acionaportaAS - open door
                me->porta_aberta = 1;
                status = Q_TRAN(&TElevador_porta_aberta); // Transition to "porta_aberta" state
            } else {
                status = Q_HANDLED(); // Continue moving
            }
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
            sendUDP(1); // acionacarroA - close door
            me->porta_aberta = 0;
            status = Q_TRAN(&TElevador_parado); // Transition to "parado" state
            break;
        }
        case PLUS1_SIG: { // Floor selection while door is open
            // Queue the request but don't move yet
            me->andar_destino = (me->andar_atual + 1) % (ANDAR_MAXIMO + 1);
            status = Q_HANDLED(); // Stay in current state
            break;
        }
        case CANCEL_SIG: { // Cancel operation
            me->andar_destino = me->andar_atual; // Cancel any destination
            status = Q_HANDLED(); // No state transition
            break;
        }
        case TIME_TICK_SIG: { // Timer tick
            // Auto-close door after 5 seconds
            static int door_open_time = 0;
            door_open_time++;
            
            if (door_open_time >= 5) {
                door_open_time = 0;
                sendUDP(1); // acionacarroA - close door
                me->porta_aberta = 0;
                
                // Check if there's a pending destination
                if (me->andar_destino != me->andar_atual) {
                    if (me->andar_destino > me->andar_atual) {
                        me->status = ELEVADOR_SUBINDO;
                        sendUDP(2); // elevadorsobeonA - elevator going up
                        sendUDP(6); // elevadorcabineonA - elevator cabin light on
                        status = Q_TRAN(&TElevador_movimento); // Transition to "movimento" state
                    } else {
                        me->status = ELEVADOR_DESCENDO;
                        sendUDP(4); // elevadordesceonA - elevator going down
                        sendUDP(6); // elevadorcabineonA - elevator cabin light on
                        status = Q_TRAN(&TElevador_movimento); // Transition to "movimento" state
                    }
                } else {
                    status = Q_TRAN(&TElevador_parado); // Transition to "parado" state
                }
            } else {
                status = Q_HANDLED(); // Stay in current state
            }
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
/* Display the current floor on the elevator's digit display */
void TElevador_display(int andar) {
    // Using the BSP_digito function to update the display with current floor
    printf("Current floor: %d\n", andar);
    fflush(stdout);
    
    // Send the current floor to the display
    // The BSP_digito function takes segment and number parameters
    // For the elevator, we'll use segment 0 to display the floor number
    BSP_digito(0, andar);
    
    // Also send the UDP signal for the display
    sendUDP(8); // elevadordigitoA
}