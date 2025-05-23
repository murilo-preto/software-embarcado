#include "bsp.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>
#include "qpc.h"
#include "safe_std.h"
#include "sinais.h"

Q_DEFINE_THIS_FILE

# define len_fila 64

static uint32_t l_rnd;
char *out_signals[] = {
    "acionaporta%d%d\0"
    "acionacarro%d\0"
    "elevadorsobeon%d\0"
    "elevadorsobeoff%d\0"
    "elevadordesceon%d\0"
    "elevadordesceoff%d\0"
    "elevadorcabineon%d\0"
    "elevadorcabineoff%d\0"
    "elevadordigito%d\0"};

char *in_signals[] = {
    "PortaAberta%d\0",
    "PortaFechada%d\0",
    "Parado%d\0",
    "sobe%d\0",
    "desce%d\0",
    "porta%d\0",
    "cabine%d\0"};

static uint32_t l_rnd;
struct sockaddr_in si_other;
int s;
WSADATA wsaData;
LPHOSTENT lpHostEntry;
pthread_t tudpServer;

static QEvt const openEvt = QEVT_INITIALIZER(OPEN_SIG);
static QEvt const sobe_botao_Evt = QEVT_INITIALIZER(SOBE_BOTAO_SIG);
static QEvt const desce_botao_Evt = QEVT_INITIALIZER(DESCE_BOTAO_SIG);
static QEvt const porta_abriu_Evt = QEVT_INITIALIZER(PORTA_ABRIU_SIG);
static QEvt const porta_fechou_Evt = QEVT_INITIALIZER(PORTA_FECHOU_SIG);
static QEvt const sinal_cabine_Evt = QEVT_INITIALIZER(CABINE_SIG);
static QEvt const sinal_parado_Evt = QEVT_INITIALIZER(PARADO_SIG);
static QEvt const sinal_andar_Evt = QEVT_INITIALIZER(ANDAR_SIG);

uint8_t andar = 0;
uint8_t fila[len_fila] = {0};

void sendUDP(int sig) {
    int slen = sizeof(si_other);
    int siglen;
    if (s != -1) {
        printf("ENVIADO: %s\n", out_signals[sig]);
        siglen = strlen(out_signals[sig]);
        sendto(s, out_signals[sig], siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}

void sendUDPSeg(int seg, int num) {
    char c;
    int slen = sizeof(si_other);
    int siglen;
    char displaySig[6];
    displaySig[0] = 'S';
    displaySig[1] = 'E';
    displaySig[2] = 'G';
    c = '0' + seg;
    displaySig[3] = c;
    c = '0' + num;
    displaySig[4] = c;
    displaySig[5] = '\0';
    if (s != -1) {
        siglen = sizeof(displaySig);
        sendto(s, displaySig, siglen, 0, (struct sockaddr *)&si_other, slen);
        printf("ENVIADO: %s\n", displaySig);
        fflush(stdout);
    }
}

void *udpServer() {
    struct sockaddr_in si_other;
    struct sockaddr_in si_me;

    int slen = sizeof(si_other), recv_len;
    char buf[BUFLEN];

    int ss = -1;
    ss = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (s != -1) {
        memset((char *)&si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(PORTIN);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(ss, (struct sockaddr *)&si_me, sizeof(si_me)) != -1) {
            while (1) {
                if ((recv_len = recvfrom(ss, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)) != -1) {
                    buf[recv_len] = '\0';
                    printf("RECEBIDO: %s\n", buf); // Adiciona um print para todos os sinais recebidos
                    fflush(stdout);

                    // Tratar sinal com prefixo "porta"
                    if (strncmp(buf, "porta", 5) == 0) {
                        int id = atoi(&buf[5]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&openEvt, NULL);
                    }

                    // Tratar sinal com prefixo "sobe"
                    if (strncmp(buf, "sobe", 4) == 0) {
                        int id = atoi(&buf[4]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&sobe_botao_Evt, NULL);
                    }

                    // Tratar sinal com prefixo "desce"
                    if (strncmp(buf, "desce", 5) == 0) {
                        int id = atoi(&buf[5]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&desce_botao_Evt, NULL);
                    }

                    // Tratar sinal com prefixo "PortaAbertaA"
                    if (strncmp(buf, "PortaAberta", 11) == 0) {
                        int id = atoi(&buf[11]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&porta_abriu_Evt, NULL);
                    }

                    // Tratar sinal com prefixo "PortaFechadaA"
                    if (strncmp(buf, "PortaFechada", 12) == 0) {
                        int id = atoi(&buf[12]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&porta_fechou_Evt, NULL);
                    }

                    // Tratar sinal com prefixo "cabine"
                    if (strncmp(buf, "cabine", 6) == 0) {
                        int id = atoi(&buf[6]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&sinal_cabine_Evt, NULL);
                    }
 
                    // Tratar sinal com prefixo "andar"
                    if (strncmp(buf, "Andar", 5) == 0) {
                        int id = atoi(&buf[5]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&sinal_andar_Evt, NULL);
                    }
  
                    // Tratar sinal com prefixo "Parado"
                    if (strncmp(buf, "Parado", 6) == 0) {
                        int id = atoi(&buf[6]);
                        andar = (uint8_t)id;
                        QACTIVE_PUBLISH(&sinal_parado_Evt, NULL);
                    }
                }
            }
        }
    }
    return NULL;
}

void bsp_on() {
    printf("on\n");
    fflush(stdout);
    sendUDP(1);
}

void bsp_off() {
    printf("off\n");
    fflush(stdout);
    sendUDP(0);
}

#ifdef Q_SPY
enum {
    PHILO_STAT = QS_USER,
    PAUSED_STAT,
};

static QSpyId const l_clock_tick = {QS_AP_ID};
#endif

Q_NORETURN Q_onError(char const *const module, int_t const id) {
    (void)module;
    (void)id;
    QS_ASSERTION(module, id, 10000U);
    QF_onCleanup();
    QS_EXIT();
    exit(-1);
}

void assert_failed(char const *const module, int_t const id);
void assert_failed(char const *const module, int_t const id) {
    Q_onError(module, id);
}

void BSP_init(int argc, char *argv[]) {
    Q_UNUSED_PAR(argc);
    Q_UNUSED_PAR(argv);
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
	pthread_create(&tudpServer,NULL,udpServer,NULL);
}

void BSP_start(void) {
    static QF_MPOOL_EL(MicroEvt) smlPoolSto[POOLSIZE];
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));
    static QEvtPtr l_microQueueSto[QUEUESIZE];
    TElevador_actor();
    QActive_start(AO_tmicro,
                  7U,
                  l_microQueueSto,
                  Q_DIM(l_microQueueSto),
                  (void *)0, 0U,
                  (void *)0);
}

void BSP_terminate(int16_t result) {
    (void)result;
    QF_stop();
}

uint32_t BSP_random(void) {
    uint32_t rnd = l_rnd * (3U * 7U * 11U * 13U * 23U);
    l_rnd = rnd;
    return rnd >> 8;
}

void BSP_randomSeed(uint32_t seed) {
    l_rnd = seed;
}

#if CUST_TICK
#include <sys/select.h>
#endif

void QF_onStartup(void) {
    QF_consoleSetup();

#if CUST_TICK
    QF_setTickRate(0U, 10U);
#else
    QF_setTickRate(BSP_TICKS_PER_SEC, 50);
#endif
}

void QF_onCleanup(void) {
    printf("\n%s\n", "Bye! Bye!");
    QF_consoleCleanup();
}

void QF_onClockTick(void) {
#if CUST_TICK
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = (1000000 / BSP_TICKS_PER_SEC);
    select(0, NULL, NULL, NULL, &tv);
#endif
    QTIMEEVT_TICK_X(0U, &l_clock_tick);
    QS_RX_INPUT();
    QS_OUTPUT();
    switch (QF_consoleGetKey()) {
        case '\33': {
            BSP_terminate(0);
            break;
        }
        default: {
            break;
        }
    }
}

#ifdef Q_SPY
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, uint32_t param2, uint32_t param3) {
    Q_UNUSED_PAR(cmdId);
    Q_UNUSED_PAR(param1);
    Q_UNUSED_PAR(param2);
    Q_UNUSED_PAR(param3);
}
#endif



void print_fila(uint8_t fila[]) {
    int length = len_fila;
    int i = 0;
    printf("Fila: ");
    for (i = 0; i < length; i++) {
        if (fila[i] != 0) {
            printf("%d ", fila[i]);
        }
    }
    printf("\n");
}


void append_fila(uint8_t fila[], uint8_t novo_andar, int dir) {
    int length = len_fila;
    int i = 0;
    for (i = 0; i < length; i++) {
        if (dir == 0){ // Se for para descer, uma ocorrencia do andar ja basta
            if (fila[i] == novo_andar){
                break;
            }
        }
        if (fila[i] == 0) {
            if (i>0 && fila[i-1] == novo_andar){
                break;
            }
            if ((i>1) && (fila[i-2]<novo_andar) && (novo_andar<fila[i-1]) && ((fila[i-1]-fila[i-2])*dir>-1)){
                // Se o elevador passa pelo novo_andar no caminho, em mesmo sentido do passageiro, adicionar parada intermediaria
                // Se o sentido for 0, o passageiro vai desembarcar do elevador
                fila[i] = fila[i-1];
                fila[i-1] = novo_andar;
                break;
            }
            fila[i] = novo_andar;
            break;
        }
    }
    print_fila(fila);
}


void atualiza_fila(uint8_t fila[]) {
    int length = len_fila;
    int i = 0;
    for (i = 0; i < length-1; i++) {
        if (fila[i] == 0) {
            break;
        }    
        else{
            fila[i] = fila[i+1];
        }
    }
    print_fila(fila);
}


int direcao_fila(uint8_t fila[], int andar_atual) {
    if (fila[0] > andar_atual) {
        return 1; // Sobe
    } else if (fila[0] < andar_atual) {
        return -1; // Desce
    }
    else {
        return 0; // Parado
    }
}


int direcao_a_para_b(int a, int b) {
    if (a < b) {
        return 1; // Sobe
    } else if (a > b) {
        return -1; // Desce
    }
    else {
        return 0; // Parado
    }
}


void BSP_porta(int andar, int modo) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (modo == 1) {
            snprintf(buffer, sizeof(buffer), "acionaporta%d+1", andar);
        }
        else if (modo == 0) {
            snprintf(buffer, sizeof(buffer), "acionaporta%d00", andar);
        }
        else {
            snprintf(buffer, sizeof(buffer), "acionaporta%d-1", andar);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        if (sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen) == -1) {
            perror("Error sending data with sendto");
        }
    }
}


void BSP_atualiza_display(int id) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        snprintf(buffer, sizeof(buffer), "elevadordigito%d", id);
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}


void BSP_ir_para_andar(int id) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        snprintf(buffer, sizeof(buffer), "acionacarro%d", id);
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}


void BSP_luz_botao_cabine(int andar, int modo) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (modo == 1) {
            snprintf(buffer, sizeof(buffer), "elevadorcabineon%d", andar);
        }
        else {
            snprintf(buffer, sizeof(buffer), "elevadorcabineoff%d", andar);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}


void BSP_desliga_botao_andar(uint8_t fila[], int id) {
    int direcao;
    if (fila[0] == 0) {
        direcao = 0;
    }
    else{
        direcao = direcao_a_para_b(id, fila[0]);
    }

    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (direcao == 1) {
            snprintf(buffer, sizeof(buffer), "elevadorsobeoff%d", id);
        }
        else if (direcao == -1){
            snprintf(buffer, sizeof(buffer), "elevadordesceoff%d", id);
        }
        else {
            snprintf(buffer, sizeof(buffer), "elevadorsobeoff%d", id);
            printf("ENVIADO: %s\n", buffer);
            siglen = strlen(buffer);
            sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);

            snprintf(buffer, sizeof(buffer), "elevadordesceoff%d", id);
            printf("ENVIADO: %s\n", buffer);
            siglen = strlen(buffer);
            sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}


void BSP_liga_botao_andar(int id, int direcao) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (direcao == 1) {
            snprintf(buffer, sizeof(buffer), "elevadorsobeon%d", id);
        }
        else {
            snprintf(buffer, sizeof(buffer), "elevadordesceon%d", id);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}