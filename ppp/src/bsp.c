#include "bsp.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

#include "qpc.h"
#include "safe_std.h"
#include "sinais.h"

Q_DEFINE_THIS_FILE

static uint32_t l_rnd;
struct sockaddr_in si_other;
int s;
WSADATA wsaData;
LPHOSTENT lpHostEntry;
pthread_t tudpServer;

// static QEvt const openEvt = QEVT_INITIALIZER(OPEN_SIG);
uint8_t id_elevador = 0;

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
                    printf("RECEBIDO: %s\n", buf);  // Adiciona um print para todos os sinais recebidos
                    fflush(stdout);

                    if (strcmp(buf, "open") == 0) {
                        MicroEvt *evt = Q_NEW(MicroEvt, OPEN_SIG);
                        QACTIVE_POST(PointA, &evt->super, NULL);
                    }

                    if (strcmp(buf, "close") == 0) {
                        MicroEvt *evt = Q_NEW(MicroEvt, CLOSE_SIG);
                        QACTIVE_POST(PointA, &evt->super, NULL);
                    }

                    if (strncmp(buf, "data:", 5) == 0) {
                        static char global_data[BUFLEN] = {0};
                        strncpy(global_data, buf + 5, BUFLEN - 1);
                        printf("DATA RECEIVED: %s\n", global_data);

                        MicroEvt *evt = Q_NEW(MicroEvt, DATA_SIG);
                        QACTIVE_POST(PointA, &evt->super, NULL);
                    }
                }
            }
        }
    }
    return NULL;
}

// ------------------------------------------------------------------------------------------------------
// Constantes do frame PPP
#define FLAG 0x7E            // Delimitador de início/fim do frame
#define ADDRESS 0xFF         // Campo Address padrão PPP
#define CONTROL 0x03         // Campo Control padrão PPP
#define PROTOCOL_LCP_H 0xC0  // Byte alto do protocolo LCP
#define PROTOCOL_LCP_L 0x21  // Byte baixo do protocolo LCP
#define MAX_FRAME_SIZE 256   // Tamanho máximo do frame (arbitrário)


//=== Função para calcular CRC-16-CCITT (polinômio 0x1021, inicializado em 0xFFFF) ===
static uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

//=== Função para aplicar escaping (byte stuffing) conforme RFC 1662 (PPP) ===
static size_t apply_byte_stuffing(const uint8_t *input, size_t in_len, uint8_t *output) {
    size_t out_index = 0;
    for (size_t i = 0; i < in_len; ++i) {
        uint8_t byte = input[i];
        // Escapa flags, escapes e control chars
        if (byte == FLAG || byte == 0x7D || byte < 0x20) {
            output[out_index++] = 0x7D;
            output[out_index++] = byte ^ 0x20;
        } else {
            output[out_index++] = byte;
        }
    }
    return out_index;
}

//=== Função principal para construir e enviar um pacote Configure-Request PPP ===
void BSP_send_configure_request(QActive *Point) {
    // Payload LCP: Code, Identifier, Length + 7 opções
    uint8_t payload[] = {
        0x01, 0x01, 0x00, 0x20,              // Code=1, ID=1, Length=32 bytes (0x20)
        0x01, 0x04, 0x05, 0xDC,              // Option MRU: 1500 (0x05DC)
        0x02, 0x06, 0xFF, 0xFF, 0xFF, 0xFF,  // Option ACCM: 0xFFFFFFFF
        0x03, 0x04, 0x00, 0x00,              // Option Auth-Prot: None
        0x04, 0x04, 0x00, 0x00,              // Option Quality-Prot: None
        0x05, 0x06, 0x12, 0x34, 0x56, 0x78,  // Option Magic-Number: fixed example
        0x07, 0x02,                          // Option Prot-Field-Comp: enabled
        0x08, 0x02                           // Option Addr-Control-Comp: enabled
    };

    // Monta frame bruto (Address, Control, Protocol, Payload)
    uint8_t frame_raw[MAX_FRAME_SIZE];
    size_t idx = 0;
    frame_raw[idx++] = ADDRESS;
    frame_raw[idx++] = CONTROL;
    frame_raw[idx++] = PROTOCOL_LCP_H;
    frame_raw[idx++] = PROTOCOL_LCP_L;
    memcpy(&frame_raw[idx], payload, sizeof(payload));
    idx += sizeof(payload);

    // Calcula e anexa FCS (2 bytes, little-endian)
    uint16_t fcs = crc16_ccitt(frame_raw, idx);
    frame_raw[idx++] = (uint8_t)(fcs & 0xFF);
    frame_raw[idx++] = (uint8_t)((fcs >> 8) & 0xFF);

    // Aplica byte stuffing ao frame bruto
    uint8_t stuffed[MAX_FRAME_SIZE * 2];
    size_t stuffed_len = apply_byte_stuffing(frame_raw, idx, stuffed);

    // Prepara frame final com flags
    static uint8_t final_frame[MAX_FRAME_SIZE * 2 + 2];
    size_t pos = 0;
    final_frame[pos++] = FLAG;  // Flag inicial
    memcpy(&final_frame[pos], stuffed, stuffed_len);
    pos += stuffed_len;
    final_frame[pos++] = FLAG;  // Flag final

    MicroEvt *evt = Q_NEW(MicroEvt, ACK_RECEIVED_SIG);
    if (evt && final_frame) {
        strncpy(evt->data, (const char *)final_frame, pos);
        evt->data[pos] = '\0';
        evt->size = (int)pos;
    }
    QACTIVE_POST(Point, &evt->super, NULL);
}

//=== Função para decodificar um frame PPP Configure-Request recebido ===
PPP_Configuration BSP_decode_configure_request(const uint8_t *frame, size_t length) {
    // Valida flags
    if (length < 2 || frame[0] != FLAG || frame[length - 1] != FLAG) {
        printf("Invalid frame: missing flags\n");
        return (PPP_Configuration){0};  // Retorna configuração vazia
    }
    // Remove flags para processamento
    size_t stuffed_len = length - 2;
    const uint8_t *stuffed = frame + 1;

    // Desfaz byte stuffing
    uint8_t raw[MAX_FRAME_SIZE];
    size_t raw_len = 0;
    for (size_t i = 0; i < stuffed_len; ++i) {
        if (stuffed[i] == 0x7D && i + 1 < stuffed_len) {
            raw[raw_len++] = stuffed[++i] ^ 0x20;
        } else {
            raw[raw_len++] = stuffed[i];
        }
    }

    // Verifica comprimento mínimo (Address, Control, Protocol, Code, ID, Length, FCS)
    if (raw_len < 4 + 4 + 2) {
        printf("Frame too short\n");
        return (PPP_Configuration){0};  // Retorna configuração vazia
    }

    // Extrai header PPP
    uint8_t address = raw[0];
    uint8_t control = raw[1];
    uint8_t proto_h = raw[2];
    uint8_t proto_l = raw[3];
    uint8_t code = raw[4];
    uint8_t identifier = raw[5];
    uint16_t len = ((uint16_t)raw[6] << 8) | raw[7];

    printf("Address=0x%02X, Control=0x%02X, Proto=0x%02X%02X\n", address, control, proto_h, proto_l);
    printf("Code=%u, ID=%u, Len=%u\n", code, identifier, len);

    // Valida FCS (últimos 2 bytes em little-endian)
    uint16_t recv_fcs = (uint16_t)raw[raw_len - 2] | ((uint16_t)raw[raw_len - 1] << 8);
    uint16_t calc_fcs = crc16_ccitt(raw, raw_len - 2);
    if (recv_fcs != calc_fcs) {
        printf("FCS mismatch: recv=0x%04X, calc=0x%04X\n", recv_fcs, calc_fcs);
        return (PPP_Configuration){0};  // Retorna configuração vazia
    }

    PPP_Configuration config;
    config.code = code;
    config.id = identifier;
    config.length = len;

    // Itera sobre as opções LCP
    size_t opt_idx = 8;        // Início das opções após Code/ID/Length
    size_t opt_end = 4 + len;  // 4 bytes de header PPP + len
    while (opt_idx + 1 < opt_end && opt_idx + 1 < raw_len - 2) {
        uint8_t type = raw[opt_idx];
        uint8_t o_len = raw[opt_idx + 1];
        if (o_len < 2 || opt_idx + o_len > opt_end) {
            printf("Invalid option length %u at idx %zu\n", o_len, opt_idx);
            break;
        }
        printf("Option Type=%u, Length=%u", type, o_len);
        switch (type) {
            case 1: {  // MRU
                config.MRU = (raw[opt_idx + 2] << 8) | raw[opt_idx + 3];
                printf(", MRU=%u\n", config.MRU);
                break;
            }
            case 2: {  // ACCM
                config.ACCM = (raw[opt_idx + 2] << 24) | (raw[opt_idx + 3] << 16) | (raw[opt_idx + 4] << 8) | raw[opt_idx + 5];
                printf(", ACCM=0x%08X\n", config.ACCM);
                break;
            }
            case 3: {  // Auth-Prot
                config.auth_prot = 0;
                printf(", none\n");
                break;
            }
            case 4: {  // Quality-Prot
                config.quality_prot = 0;
                printf(", none\n");
                break;
            }
            case 5: {  // Magic Number
                config.magic_number = (raw[opt_idx + 2] << 24) | (raw[opt_idx + 3] << 16) | (raw[opt_idx + 4] << 8) | raw[opt_idx + 5];
                printf(", Magic=0x%08X\n", config.magic_number);
                break;
            }
            case 7: {  // Prot-Field-Comp
                config.protocol_field_comp = 1;
                printf(", enabled\n");
                break;
            }
            case 8: {  // Addr-Control-Comp
                config.addr_control_comp = 1;
                printf(", enabled\n");
                break;
            }
            default: {
                printf(", data omitted\n");
                break;
            }
        }
        opt_idx += o_len;
    }

    return config;
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
    QS_GLB_FILTER(-QS_QF_TICK);  // exclude the tick record

    WSAStartup(MAKEWORD(2, 1), &wsaData);
    lpHostEntry = gethostbyname("127.0.0.1");
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORTOUT);
    si_other.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
    pthread_create(&tudpServer, NULL, udpServer, NULL);
}

void BSP_start(void) {
    static QF_MPOOL_EL(MicroEvt) smlPoolSto[POOLSIZE];
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));
    // static QEvtPtr l_microQueueSto[QUEUESIZE];

    printf("Instatiating actors...\n");

    Point_actor(1);
    Point_actor(2);
    Point_actor(3);

    printf("QActive start...\n");

    static QEvtPtr l_microQueueSto1[QUEUESIZE];  // Separate queues
    static QEvtPtr l_microQueueSto2[QUEUESIZE];

    QActive_start(PointA,
                  1U,  // Unique priority (1, 2, 3)
                  l_microQueueSto1,
                  Q_DIM(l_microQueueSto1),
                  (void *)0, 0U,
                  (void *)0);

    QActive_start(PointB,
                  2U,  // Unique priority
                  l_microQueueSto2,
                  Q_DIM(l_microQueueSto2),
                  (void *)0, 0U,
                  (void *)0);

    printf("Ran all start procuderes...\n");
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

void BSP_porta(int id, int direcao) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (direcao == 1) {
            snprintf(buffer, sizeof(buffer), "acionaporta%d+1", id);
        } else if (direcao == 0) {
            snprintf(buffer, sizeof(buffer), "acionaporta%d00", id);
        } else {
            snprintf(buffer, sizeof(buffer), "acionaporta%d-1", id);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        if (sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen) == -1) {
            perror("Error sending data with sendto");
        }
    }
}

void BSP_botao_sobe(int id) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        snprintf(buffer, sizeof(buffer), "elevadorsobeon%d", id);
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}

void BSP_botao_desce(int id) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        snprintf(buffer, sizeof(buffer), "elevadordesceon%d", id);
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}

void BSP_porta_abriu(int id, int direcao) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (direcao == 1) {
            snprintf(buffer, sizeof(buffer), "elevadorsobeoff%d", id);
        } else {
            snprintf(buffer, sizeof(buffer), "elevadordesceoff%d", id);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}

void BSP_cabine(int id, int mode) {
    int slen = sizeof(si_other);
    int siglen;
    char buffer[50];
    if (s != -1) {
        if (mode == 1) {
            snprintf(buffer, sizeof(buffer), "elevadorcabineon%d", id);
        } else {
            snprintf(buffer, sizeof(buffer), "elevadorcabineoff%d", id);
        }
        printf("ENVIADO: %s\n", buffer);
        siglen = strlen(buffer);
        sendto(s, buffer, siglen, 0, (struct sockaddr *)&si_other, slen);
    }
}
