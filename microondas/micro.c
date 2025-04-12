/*
 * micro.c
 * Implementation of the microwave control system using a hierarchical state machine.
 */

#include "sinais.h"
#include "bsp.h"
#include <stdio.h>

// Forward declaration for TMicro_display
void TMicro_display(int t);

// Define the duration of one second in terms of system ticks
#define UM_SEG (QTimeEvtCtr)(BSP_TICKS_PER_SEC)

// Internal signals (extend the signal enumeration)
enum InternalSignals {
    TIMEOUT_SIG = MAX_SIG // Timeout signal
};

// Active object structure for the microwave
typedef struct TMicroTag {
    QActive super;       // Base class (inherits from QActive)
    int t;               // Cooking time in seconds
    QTimeEvt timeEvt;    // Time event for periodic signals
} TMicro;

// Static instance of the microwave active object
static TMicro l_TMicro;

// Forward declarations of state handler functions
static QState TMicro_off(TMicro * const me, QEvt const * const e);
static QState TMicro_on(TMicro * const me, QEvt const * const e);
static QState TMicro_initial(TMicro * const me, QEvt const *e);
static QState TMicro_on_door_open(TMicro * const me, QEvt const * const e);

// Global pointer to the active object
QActive * const AO_tmicro = (QActive *)&l_TMicro;

// Constructor for the microwave active object
void TMicro_ctor() {
    TMicro * const me = &l_TMicro;
    QActive_ctor(&me->super, Q_STATE_CAST(&TMicro_initial)); // Initialize the base class
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U); // Initialize the time event
}

/*..........................................................................*/
/* Initial state of the microwave */
QState TMicro_initial(TMicro * const me, QEvt const *e) {
    (void)e; // Suppress unused parameter warning

    // Initialize hardware and variables
    BSP_forno(0); // Turn off the microwave oven
    BSP_luz(0);   // Turn off the light
    me->t = 0;    // Reset cooking time
    TMicro_display(me->t); // Display the initial time

    // Subscribe to relevant signals
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);
    QActive_subscribe((QActive *)me, PLUS1_SIG);
    QActive_subscribe((QActive *)me, OPEN_SIG);
    QActive_subscribe((QActive *)me, CLOSE_SIG);
    QActive_subscribe((QActive *)me, CANCEL_SIG);

    // Arm the time event for periodic signals
    QTimeEvt_armX(&me->timeEvt, UM_SEG, 0);

    // Transition to the "off" state
    return Q_TRAN(&TMicro_off);
}

/*..........................................................................*/
/* State: Microwave is off */
QState TMicro_off(TMicro * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Signal received in TMicro_off: %d\n", e->sig);

    switch (e->sig) {
        case OPEN_SIG: { // Door opened
            BSP_forno(0); // Ensure the oven is off
            BSP_luz(1);   // Turn on the light
            status = Q_TRAN(&TMicro_on_door_open); // Transition to "door open" state
            break;
        }
        case CLOSE_SIG: { // Door closed
            status = Q_HANDLED(); // No state transition
            break;
        }
        case PLUS1_SIG: { // Add one minute
            BSP_forno(1); // Turn on the oven
            BSP_luz(1);   // Turn on the light
            status = Q_TRAN(&TMicro_on); // Transition to "on" state
            break;
        }
        case CANCEL_SIG: { // Cancel operation
            status = Q_HANDLED(); // No state transition
            break;
        }
        case TIME_TICK_SIG: { // Timer tick
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
/* State: Microwave is on */
QState TMicro_on(TMicro * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Signal received in TMicro_on: %d\n", e->sig);

    switch (e->sig) {
        case OPEN_SIG: { // Door opened
            BSP_forno(0); // Turn off the oven
            BSP_luz(1);   // Keep the light on
            status = Q_TRAN(&TMicro_on_door_open); // Transition to "door open" state
            break;
        }
        case CLOSE_SIG: { // Door closed
            status = Q_HANDLED(); // No state transition
            break;
        }
        case PLUS1_SIG: { // Add one minute
            me->t += 60; // Add 60 seconds to the timer
            TMicro_display(me->t); // Update the display
            status = Q_HANDLED(); // No state transition
            break;
        }
        case CANCEL_SIG: { // Cancel operation
            BSP_forno(0); // Turn off the oven
            BSP_luz(0);   // Turn off the light
            status = Q_TRAN(&TMicro_off); // Transition to "off" state
            break;
        }
        case TIME_TICK_SIG: { // Timer tick
            if (me->t > 0) {
                me->t--; // Decrement the timer
                TMicro_display(me->t); // Update the display
                if (me->t == 0) {
                    BSP_forno(0); // Turn off the oven when time is up
                    BSP_luz(0);   // Turn off the light
                    status = Q_TRAN(&TMicro_off); // Transition to "off" state
                } else {
                    status = Q_HANDLED(); // No state transition
                }
            } else {
                status = Q_HANDLED(); // No state transition
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
/* State: Door is open */
static QState TMicro_on_door_open(TMicro * const me, QEvt const * const e) {
    QState status;

    // Log the received signal
    printf("Signal received in TMicro_on_door_open: %d\n", e->sig);

    switch (e->sig) {
        case CLOSE_SIG: { // Door closed
            if (me->t > 0) {
                BSP_forno(1); // Turn on the oven
                BSP_luz(1);   // Keep the light on
                status = Q_TRAN(&TMicro_on); // Transition to "on" state
            } else {
                BSP_luz(0); // Turn off the light
                status = Q_TRAN(&TMicro_off); // Transition to "off" state
            }
            break;
        }
        case CANCEL_SIG: { // Cancel operation
            me->t = 0; // Reset the timer
            BSP_forno(0); // Turn off the oven
            BSP_luz(0);   // Turn off the light
            status = Q_TRAN(&TMicro_off); // Transition to "off" state
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
/* Display the remaining cooking time */
void TMicro_display(int t) {
    printf("Time remaining: %d seconds\n", t);
    fflush(stdout);
}