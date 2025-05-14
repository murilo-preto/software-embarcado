#include <stdio.h>

#include "bsp.h"
#include "sinais.h"

void TElevador_display(int andar);
#define UM_SEG (QTimeEvtCtr)(BSP_TICKS_PER_SEC)

enum InternalSignals {
    TIMEOUT_SIG = MAX_SIG
};

typedef struct PointTag {
    QActive super;
    int id;
    QTimeEvt timeEvt;
    PPP_Configuration config;
    char point_id;
} Point;

static Point l_PointA, l_PointB;

QActive *const PointA = (QActive *)&l_PointA;
QActive *const PointB = (QActive *)&l_PointB;

static QState S_STARTING(Point *const me, QEvt const *e);
static QState S_LISTENING(Point *const me, QEvt const *e);
static QState S_REQ_SENT(Point *const me, QEvt const *const e);
static QState S_ACK_REC(Point *const me, QEvt const *const e);
static QState S_ACK_SENT(Point *const me, QEvt const *const e);
static QState S_OPENED(Point *const me, QEvt const *const e);

void print_hex_string(const char *hex_str) {
    for (size_t i = 0; hex_str[i] && hex_str[i + 1]; i += 2) {
        unsigned int byte;
        sscanf(&hex_str[i], "%2x", &byte);
        printf("%02X ", byte);
    }
    printf("\n");
}

// Compare all fields of the PPP_Configuration struct
bool PPP_is_config_match(const PPP_Configuration *configA, const PPP_Configuration *configB) {
    if (configA == NULL || configB == NULL) {
        return false;
    }

    return (configA->code == configB->code) &&
           (configA->id == configB->id) &&
           (configA->length == configB->length) &&
           (configA->MRU == configB->MRU) &&
           (configA->ACCM == configB->ACCM) &&
           (configA->auth_prot == configB->auth_prot) &&
           (configA->quality_prot == configB->quality_prot) &&
           (configA->magic_number == configB->magic_number) &&
           (configA->protocol_field_comp == configB->protocol_field_comp) &&
           (configA->addr_control_comp == configB->addr_control_comp);
}

void Point_actor(int id) {
    Point *me;
    switch (id) {
        case 1:
            me = &l_PointA;
            break;
        case 2:
            me = &l_PointB;
            break;
        default:
            return;
    }
    me->id = id;
    QActive_ctor(&me->super, Q_STATE_CAST(&S_STARTING));
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIME_TICK_SIG, 0U);
}

/* Initial state of the point */
QState S_STARTING(Point *const me, QEvt const *e) {
    (void)e;  // Suppress unused parameter warning

    me->config.code = 1;
    me->config.id = 1;
    me->config.length = 32;
    me->config.MRU = 1500;
    me->config.ACCM = 0xFFFFFFFF;
    me->config.auth_prot = 0;
    me->config.quality_prot = 0;
    me->config.magic_number = 0x12345678;
    me->config.protocol_field_comp = 1;
    me->config.addr_control_comp = 1;

    me->point_id = '0';

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
QState S_LISTENING(Point *const me, QEvt const *const e) {
    QState status = Q_HANDLED();  // Default to handled state

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            printf(" S_LISTENING: ENTRY_SIG \n");
            status = Q_HANDLED();
            break;
        }
        case OPEN_SIG: {
            printf(" S_LISTENING: OPEN_SIG \n");
            BSP_send_configure_request(PointB);

            me->point_id = 'A';
            printf("%c: LISTENING -> S_REQ_SENT\n", me->point_id);
            status = Q_TRAN(S_REQ_SENT);
            break;
        }
        case ACK_RECEIVED_SIG: {
            printf(" S_LISTENING: ACK_RECEIVED_SIG \n");
            PPP_Configuration config = BSP_decode_configure_request(((MicroEvt *)e)->data, ((MicroEvt *)e)->size);

            if (PPP_is_config_match(&me->config, &config)) {
                printf("Config accepted\n");
 
                me->point_id = 'B';
                printf("%c: LISTENING -> S_ACK_REC\n", me->point_id);               
                status = Q_TRAN(S_ACK_REC);
            } else {
                printf("Condig denied\n");
                status = Q_HANDLED();
            }

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
QState S_REQ_SENT(Point *const me, QEvt const *const e) {
    QState status = Q_HANDLED();  // Default to handled state

    switch (e->sig) {
        case Q_INIT_SIG: {
            printf("%c: S_REQ_SENT: Q_INIT_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        case Q_ENTRY_SIG: {
            printf("%c: S_REQ_SENT: ENTRY_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        case DATA_SIG: {
            printf("%c: S_REQ_SENT: DATA_SIG \n", me->point_id);
            char *data = ((MicroEvt *)e)->data;
            print_hex_string(data);
            char* message = BSP_extract_ppp_payload((uint8_t *)data, ((MicroEvt *)e)->size);
            printf("%c: Message received: %s\n", me->point_id, message);
            status = Q_HANDLED();
            break;
        }
        case OPEN_SIG: {
            printf("%c: S_REQ_SENT: OPEN_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        case CLOSE_SIG: {
            printf("%c: S_REQ_SENT: CLOSE_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        case TIME_TICK_SIG: {
            printf("%c: S_REQ_SENT: TIME_TICK_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        default: {
            printf("%c: S_REQ_SENT: Unhandled signal %d\n", me->point_id, e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}

/*..........................................................................*/
QState S_ACK_REC(Point *const me, QEvt const *const e) {
    QState status;

    switch (e->sig) {
        case Q_INIT_SIG: {
            printf("%c: S_ACK_REC: Q_INIT_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        case Q_ENTRY_SIG: {
            printf("%c: S_ACK_REC: ENTRY_SIG \n", me->point_id);
            BSP_send_ppp_data(PointA, "ACK");
            printf("%c: ACK sent\n", me->point_id);

            status = Q_HANDLED();
            break;
        }
        case OPEN_SIG: {
            printf("%c: S_ACK_REC: OPEN_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        case CLOSE_SIG: {
            printf("%c: S_ACK_REC: CLOSE_SIG \n", me->point_id);
            status = Q_HANDLED();
            break;
        }
        default: {
            printf("%c: S_ACK_REC: Unhandled signal %d\n", me->point_id, e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}

/*..........................................................................*/
/* State: Elevator door is open */
static QState S_ACK_SENT(Point *const me, QEvt const *const e) {
    QState status;

    switch (e->sig) {
        default: {
            printf("Unhandled signal in S_ACK_SENT: %d\n", e->sig);
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}