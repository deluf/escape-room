
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "protocol.h"

const char* const action_to_str[] = {
    "SERVER",
    "CLIENT",
    "HELP",
    "QUESTION",
    "ANSWER",
    "START",
    "LOOK",
    "TAKE",
    "USE",
    "OBJS",
    "DROP",
    "END",
    "ACTION_MAX"
};

enum ACTION str_to_action(char *buffer) {
    if (strcmp(buffer, "start") == 0) {
        return START;
    }
    if (strcmp(buffer, "look") == 0) {
        return LOOK;
    }
    if (strcmp(buffer, "take") == 0) {
        return TAKE;
    }
    if (strcmp(buffer, "use") == 0) {
        return USE;
    }
    if (strcmp(buffer, "objs") == 0) {
        return OBJS;
    }
    if (strcmp(buffer, "drop") == 0) {
        return DROP;
    }
    if (strcmp(buffer, "end") == 0) {
        return END;
    }
    return HELP;
}

const char* const response_to_str[] = {
    "CREDENTIALS_TOO_SHORT",
    "CREDENTIALS_TOO_LONG",
    "LOGIN_FAIL",
    "ALREADY_LOGGED_IN",
    "LOGIN_SUCCESS",
    "REGISTERED",
    "SERVER_FULL"
};

int encode_message(char *buffer, int size, enum ACTION action, int argc, char *argv[ARGC_MAX]) {

    int argi, i, j;

    buffer[0] = (uint8_t)action;
    i = 1;  /* Numero di byte scritti in *buffer* */

    /* Il messaggio è composto solamente dall'azione */
    if (argc == 0) {
        return i;
    }

    for (argi = 0; argi < argc; argi++) {
        j = 0;
        while (i < size) {
            char c;
            c = argv[argi][j];
            if (c == '\0') {
                /* Se è l'ultimo argomento scrivi '\\0', altrimenti *SEPARATOR* */
                if (argi == argc - 1) {
                    buffer[i] = '\0';
                    return i + 1;
                }
                buffer[i] = SEPARATOR;
                break;
            }
            buffer[i] = c;
            i++;
            j++;
        }
        i++;   
    }

    /* E' stata raggiunta *size* senza aver completato la scrittura degli argomenti */
    return -1;
}

int decode_message(const char *buffer, int size, enum ACTION *action, int *argc, char *argv[ARGC_MAX]) {
    
    int i, j;

    /* Vanno fatti dei controlli perchè il messaggio arriva dalla rete */
    if (buffer[0] < 0 || buffer[0] >= ACTION_MAX) {
        return -1;
    }

    if (action != NULL) {
        *action = buffer[0];
    }

    *argc = 0;
    memset(argv, 0, sizeof(char *) * ARGC_MAX);

    /* Il messaggio è composto solamente dall'azione */
    if (size == 1) {
        return 0;
    }

    i = 1;  /* Abbiamo già interpretato il primo byte */
    j = 0;
    while (i < size) {
        char c = buffer[i];

        if (c == SEPARATOR || c == '\0') {
            int k;

            /**
             * In questo momento j+1 è la dimensione del vettore da allocare
             * e buffer[i - j], ..., buffer[i-1], '\\0' sono i byte da scriverci
             */
            argv[*argc] = malloc(j + 1);
            for (k = 0; k < j; k++) {
                argv[*argc][k] = buffer[i - j + k];
            }
            argv[*argc][j] = '\0';

            (*argc)++;
            i++;
            j = 0;

            if (c == SEPARATOR) {
                continue;
            }
            return 0;
        }

        i++;
        j++;
    }

    /* E' stata raggiunta *size* senza aver completato la scrittura degli argomenti */
    return -1;
}

void free_argv(char *argv[ARGC_MAX]) {
    int i;
    for (i = 0; i < ARGC_MAX; i++) {
        free(argv[i]); /* free(NULL) non è un errore */
    }
}

int send_msg(int sd, enum ACTION action, int argc, char *argv[ARGC_MAX]) {

    char buffer[IO_BUFFER_SIZE];
    uint16_t n_length, h_length;
    int ret;

#ifdef NDEBUG
    printf("\n\t#SENT\n\taction: %d\n\targc: %d\n\targv[0]: %s\n\targv[1]: %s\n", action, argc, argv[0], argv[1]);
#endif

    /* Codifica del messaggio */
    ret = encode_message(buffer, IO_BUFFER_SIZE, action, argc, argv);
    h_length = ret;
    n_length = htons(h_length);

#ifdef NDEBUG
    printf("\n\t#RAW BUFFER SENT\n\tlength: %d\n\taction: %d\n\tbuffer: %s\n\tret: %d\n", h_length, buffer[0], buffer + 1, ret);
#endif

    /* Invio (su 2 byte) della dimensione del messaggio codificato */
    ret = send(sd, &n_length, sizeof(n_length), MSG_NOSIGNAL);
    if (ret == -1) {
        return -1;
    }

    /* Invio del messaggio codificato */
    ret = send(sd, &buffer, h_length, MSG_NOSIGNAL);
    if (ret == -1) {
        return -1;
    }

    return 0;
}

int recv_msg(int sd, enum ACTION *action, int *argc, char *argv[ARGC_MAX]) {

    char buffer[IO_BUFFER_SIZE];
    uint16_t n_length, h_length;
    int ret;

    /* Ricezione della dimensione del messaggio codificato */
    ret = recv(sd, &n_length, sizeof(n_length), 0);
    if (ret <= 0) {
        return -1;
    }

    h_length = ntohs(n_length);
    if (h_length > IO_BUFFER_SIZE) {
        return -1;
    }

    /* Ricezione del messaggio codificato */
    ret = recv(sd, &buffer, h_length, 0);
    if (ret <= 0) {
        return -1;
    }

#ifdef NDEBUG
    printf("\n\t#RAW BUFFER RECEIVED\n\tlength: %d\n\taction: %d\n\tbuffer: %s\n", h_length, buffer[0], buffer + 1);
#endif

    ret = decode_message(buffer, h_length, action, argc, argv);

#ifdef NDEBUG
    if (action == NULL) { printf("\n\t#RECEIVED\n\taction: NULL\n\targc: %d\n\targv[0]: %s\n\targv[1]: %s\n\tret: %d\n", *argc, argv[0], argv[1], ret); }
    else {  printf("\n\t#RECEIVED\n\taction: %d\n\targc: %d\n\targv[0]: %s\n\targv[1]: %s\n\tret: %d\n", *action, *argc, argv[0], argv[1], ret); }
#endif

    return ret;
}
