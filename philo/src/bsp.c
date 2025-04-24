//============================================================================
// Product: BSP for DPP example (console)
// Last updated for version 8.0.0
// Last updated on  2024-09-18
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <www.gnu.org/licenses/>.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#include "qpc.h"      // QP/C real-time embedded framework
#include "dpp.h"      // DPP Application interface
#include "bsp.h"      // Board Support Package

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>


#include "safe_std.h" // portable "safe" <stdio.h>/<string.h> facilities
#include <stdlib.h>   // for exit()

Q_DEFINE_THIS_FILE

// Local objects -------------------------------------------------------------
static uint32_t l_rnd; // random seed
char *pensando = "filo0Pensando\0";
char *comendo = "filo0Comendo \0";
char *comfome = "filo0Com Fome\0";


static uint32_t l_rnd;
struct sockaddr_in si_other;
int s;
WSADATA wsaData;
LPHOSTENT lpHostEntry;

void sendUDP(int sig, int philo){
	char *signal;
	char buf[BUFLEN];
	char philoID;

	int slen = sizeof(si_other);
	int siglen;

	//printf("Filo  %d sig %d\n",philo, sig);

	if(s != -1){
    // zero out the structure
		switch(sig)
		{
			case 2:
				signal = comfome;
				break;
			case 1:
				signal = comendo;
				break;
			default:
				signal = pensando;
				break;
		}
		strcpy(buf, signal);
		philoID = '0';
		philoID += philo;
		buf[4] = philoID;
		siglen = strlen(buf);
		sendto(s, buf, siglen, 0, (struct sockaddr*) &si_other, slen);
	}
}

#ifdef Q_SPY
    enum {
        PHILO_STAT = QS_USER,
        PAUSED_STAT,
    };

    // QSpy source IDs
    static QSpyId const l_clock_tick = { QS_AP_ID };
#endif

//============================================================================
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    QS_ASSERTION(module, id, 10000U); // report assertion to QS
    QF_onCleanup();
    QS_EXIT();
    exit(-1);
}
//............................................................................
void assert_failed(char const * const module, int_t const id); // prototype
void assert_failed(char const * const module, int_t const id) {
    Q_onError(module, id);
}

//============================================================================
void BSP_init(int argc, char *argv[]) {
    Q_UNUSED_PAR(argc);
    Q_UNUSED_PAR(argv);

    printf("Dining Philosophers Problem example"
           "\nQP %s\n"
           "Press 'p' to pause\n"
           "Press 's' to serve\n"
           "Press ESC to quit...\n",
           QP_VERSION_STR);

    BSP_randomSeed(1234U);

    // initialize the QS software tracing
    if (QS_INIT((argc > 1) ? argv[1] : (void *)0) == 0U) {
        Q_ERROR();
    }

    QS_OBJ_DICTIONARY(&l_clock_tick);

    QS_USR_DICTIONARY(PHILO_STAT);
    QS_USR_DICTIONARY(PAUSED_STAT);

    QS_ONLY(produce_sig_dict());

    // setup the QS filters...
    QS_GLB_FILTER(QS_ALL_RECORDS);
    QS_GLB_FILTER(-QS_QF_TICK);    // exclude the tick record

	WSAStartup(MAKEWORD(2,1),&wsaData);
    lpHostEntry = gethostbyname("127.0.0.1");
	s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORTOUT);
	si_other.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
}
//............................................................................
void BSP_start(void) {
    // initialize event pools
    static QF_MPOOL_EL(TableEvt) smlPoolSto[2*N_PHILO];
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // instantiate and start AOs/threads...

    static QEvtPtr philoQueueSto[N_PHILO][10];
    for (uint8_t n = 0U; n < N_PHILO; ++n) {
        Philo_ctor(n);
        QActive_start(AO_Philo[n],
            n + 3U,                  // QP prio. of the AO
            philoQueueSto[n],        // event queue storage
            Q_DIM(philoQueueSto[n]), // queue length [events]
            (void *)0, 0U,           // no stack storage
            (void *)0);              // no initialization param
    }

    static QEvtPtr tableQueueSto[N_PHILO];
    Table_ctor();
    QActive_start(AO_Table,
        N_PHILO + 7U,            // QP prio. of the AO
        tableQueueSto,           // event queue storage
        Q_DIM(tableQueueSto),    // queue length [events]
        (void *)0, 0U,           // no stack storage
        (void *)0);              // no initialization param
}
//............................................................................
void BSP_terminate(int16_t result) {
    (void)result;
    QF_stop(); // stop the main "ticker thread"
}
//............................................................................
void BSP_displayPhilStat(uint8_t n, uint8_t st, char const *stat) {
    printf("Philosopher %2d is %s\n", (int)n, stat);
    sendUDP(st, n);
    fflush(stdout);
    // application-specific record
    QS_BEGIN_ID(PHILO_STAT, AO_Table->prio)
        QS_U8(1, n);  // Philosopher number
        QS_STR(stat); // Philosopher status
    QS_END()
}
//............................................................................
void BSP_displayPaused(uint8_t paused) {
    printf("Paused is %s\n", paused ? "ON" : "OFF");
}
//............................................................................
uint32_t BSP_random(void) { // a very cheap pseudo-random-number generator
    // "Super-Duper" Linear Congruential Generator (LCG)
    // LCG(2^32, 3*7*11*13*23, 0, seed)
    //
    uint32_t rnd = l_rnd * (3U*7U*11U*13U*23U);
    l_rnd = rnd;
    return rnd >> 8;
}
//............................................................................
void BSP_randomSeed(uint32_t seed) {
    l_rnd = seed;
}

//============================================================================
#if CUST_TICK
#include <sys/select.h> // for select() call used in custom tick processing
#endif

void QF_onStartup(void) {
    QF_consoleSetup();

#if CUST_TICK
    // disable the standard clock-tick service by setting tick-rate to 0
    QF_setTickRate(0U, 10U); // zero tick-rate / ticker thread prio.
#else
    QF_setTickRate(BSP_TICKS_PER_SEC, 10); // desired tick rate/ticker-prio
#endif
}
//............................................................................
void QF_onCleanup(void) {
    printf("\n%s\n", "Bye! Bye!");
    QF_consoleCleanup();
}
//............................................................................
void QF_onClockTick(void) {

#if CUST_TICK
    // NOTE:
    // The standard clock-tick service has been DISABLED in QF_onStartup()
    // by setting the clock tick rate to zero.
    // Therefore QF_onClockTick() must implement an alternative waiting
    // mechanism for the clock period. This particular implementation is
    // based on the select() system call to block for the desired timeout.

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = (1000000/BSP_TICKS_PER_SEC);
    select(0, NULL, NULL, NULL, &tv); // block for the timevalue
#endif

    QTIMEEVT_TICK_X(0U, &l_clock_tick); // process time events at rate 0

    QS_RX_INPUT(); // handle the QS-RX input
    QS_OUTPUT();   // handle the QS output

    switch (QF_consoleGetKey()) {
        case '\33': { // ESC pressed?
            BSP_terminate(0);
            break;
        }
        case 'p': {
            static QEvt const pauseEvt = QEVT_INITIALIZER(PAUSE_SIG);
            QACTIVE_PUBLISH(&pauseEvt, &l_clock_tick);
            break;
        }
        case 's': {
            static QEvt const serveEvt = QEVT_INITIALIZER(SERVE_SIG);
            QACTIVE_PUBLISH(&serveEvt, &l_clock_tick);
            break;
        }
        default: {
            break;
        }
    }
}

//============================================================================
#ifdef Q_SPY // define QS callbacks

//............................................................................
//! callback function to execute user commands
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, uint32_t param2, uint32_t param3)
{
    Q_UNUSED_PAR(cmdId);
    Q_UNUSED_PAR(param1);
    Q_UNUSED_PAR(param2);
    Q_UNUSED_PAR(param3);
}

#endif // Q_SPY
