#include "qpc.h"          // QP/C real-time embedded framework
#include "bsp.h"          // Board Support Package

//............................................................................
int main(int argc, char *argv[]) {
    QF_init();            // initialize the framework
    BSP_init(argc, argv); // initialize the BSP
    BSP_start();          // start the AOs/Threads
    return QF_run();      // run the QF application (contains the main loop)
}
