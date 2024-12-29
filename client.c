
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "lib/protocol.h"
#include "lib/mystdlib.h"

#define SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 4242

/* Massimo numero di tentativi di connessione da effettuare all'inizio */
#define INIT_CONNECTION_MAX_TRIES 10

/* Massimo numero di argomenti di un qualsiasi comando del client */
#define ARGC_CLIENT_MAX 2

/**
 * Invia al server il messaggio di accesso.
 * Ritorna la risposta ricevuta dal server.
 * In caso di errore ritorna -1.
 */
enum RESPONSE login(int sd, char *username, char *password) {

    enum RESPONSE response;
    char *argv[2];
    int ret;

    argv[0] = username;
    argv[1] = password;

    ret = send_msg(sd, CLIENT, 2, argv);
    if (ret == -1) {
        return -1;
    }

    /* Ricezione della risposta del server (la dimensione è nota) */
    ret = recv(sd, &response, sizeof(response), 0);
    if (ret <= 0) {
        return -1;
    }

    return ntohl(response);
}

/**
 * Crea un socket per comunicare con il server e la porta specificati.
 * La connessione al server viene tentata *INIT_CONNECTION_MAX_TRIES*
 *  volte, ad intervalli di 1 secondo.
 * Ritorna il socket descriptor usato per la comunicazione.
 * In caso di errore ritorna -1.
 */
int init_connection(const char *server_ip, uint16_t server_port) {

    int sd, i, ret;
    struct sockaddr_in server_addr;

    /* Creazione del socket */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        perror_fatal();
        exit(-1);
    }

    /* Definizione del server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    /* Connessione al server */
    for (i = 0; i < INIT_CONNECTION_MAX_TRIES; i++) {
        ret = connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (ret == 0) {
            return sd;
        }
        sleep(1);
    }

    return -1;
}

/**
 * Stampa la descrizione del comando specificato in *action*.
 * Se *action* è HELP stampa la lista dei comandi disponibili.
 */
void print_help(enum ACTION action) {
    switch (action) {
        case START:
            printf(
                " # Sintassi: start room\n"
                " Entra nella stanza di numero *room*\n");
            break;
        case LOOK:
            printf(
                " # Sintassi: look [location | object]\n"
                " Descrive la stanza, o la locazione/oggetto"
                " eventualmente specificati\n");
            break;
        case TAKE:
            printf(
                " # Sintassi: take object\n"
                " Raccoglie l'oggetto specificato\n");
            break;
        case USE:
            printf(
                " # Sintassi: use object1 [object2]\n"
                " Utilizza il primo oggetto singolarmente o"
                " in coppia con il secondo\n");
            break;
        case OBJS:
            printf(
                " # Sintassi: objs\n"
                " Mostra la lista degli oggetti raccolti "
                "fin'ora\n");
            break;
        case DROP:
            printf(
                " # Sintassi: drop object\n"
                " Posa l'oggetto precedentemente raccolto\n");
            break;
        case END:
            printf(
                " # Sintassi: end\n"
                " Termina il gioco\n");
            break;
        default:
            printf(
                " Lista dei comandi disponibili:\n"
                "  > start room\n"
                "  > look [location | object]\n"
                "  > take object\n"
                "  > use object1 [object2]\n"
                "  > objs\n"
                "  > drop\n"
                "  > end\n"
                " Per una descrizione più accurata puoi scrivere > help"
                    " comando\n");
            break;
    }
}

/**
 * Legge lo standard input interpretandone i caretteri 
 *  come un comando per il client. I possibili comandi
 *  coincidono (quasi) con le azioni definite in enum ACTIONS
 *  di protocol.h.
 * 
 * La prima parola terminata da ' ' o '\\0' viene convertita in ACTION,
 *  ogni parola successiva finisce in un *argv*, il numero totale delle
 *  parole tradotte va in *argc*.
 * 
 * Se ci sono più di *argc_max* parole le ignora.
 * I buffer in *argv* devono deallocati con free-argv(...) dopo l'utilizzo.
 */ 
void parse_command(enum ACTION *action, int *argc, char *argv[ARGC_MAX], int argc_max) {
    
    char buffer[IO_BUFFER_SIZE];
    char command[IO_BUFFER_SIZE];
    int i, j;

    fgetsnn(buffer, IO_BUFFER_SIZE, stdin);

    /* Versione leggermente modificata della decode_message(...) */
    *argc = 0;
    memset(argv, 0, sizeof(char *) * ARGC_MAX);

    i = 0;
    j = 0;
    while (i < IO_BUFFER_SIZE) {
        char c = buffer[i];

        if (c == ' ' || c == '\0') {
            int k;

            /* Ogni argomento deve contenere almeno un byte oltre a '\\0' */
            if (j < 1) {
                break;
            }

            /**
             * In questo momento j+1 è la dimensione del vettore da allocare
             * e buffer[i - j], ..., buffer[i-1], '\\0' sono i byte da scriverci
             */

            /* La prima parola (il comando) non copiarla in *argv* ma in *command* */
            if (i == j) {
                for (k = 0; k < j; k++) {
                    command[k] = buffer[i - j + k];
                }
                command[j] = '\0';
            }
            else {
                argv[*argc] = malloc(j + 1);
                for (k = 0; k < j; k++) {
                    argv[*argc][k] = buffer[i - j + k];
                }
                argv[*argc][j] = '\0';

                (*argc)++;
                if  (*argc == argc_max) {
                    break;
                }
            }

            i++;
            j = 0;

            /* Per quanto riguarda i comandi del client il separatore è uno spazio */
            if (c == ' ') { 
                continue;
            }
            break; /* Raggiunto '\\0' */
        }

        i++;
        j++;
    }

    /* Qualcosa è andato storto (impossibile aver processato meno di un carattere) */
    if (i <= 1) {
        *action = HELP;
        return;
    }

    *action = str_to_action(command);
}

int main(int argc, char *argv[]) {

    enum RESPONSE response;
    int sd;

    sd = init_connection(SERVER_IP, DEFAULT_SERVER_PORT);
    if (sd == -1) {
        printf(ANSI_COLOR_RED " [Errore]: Impossibile connettersi al server\n" ANSI_COLOR_RESET);
        exit(-1);
    }

    printf("\n############################## INTERFACCIA CLIENT ##############################\n\n");
    printf(" Accedi inserendo username e password\n");
    printf(" Se non hai un account questo verrà registrato automaticamente\n");
    printf(" (Username e passowrd devono essere compresi tra %d e %d caratteri)\n",
        CREDENTIALS_LENGTH_MIN - 1, CREDENTIALS_LENGTH_MAX - 1);

    do {
        char username[CREDENTIALS_LENGTH_MAX];
        char password[CREDENTIALS_LENGTH_MAX];

        printf("\n");
        printf(" > username: ");
        fgetsnn(username, CREDENTIALS_LENGTH_MAX, stdin);
        printf(" > password: ");
        fgetsnn(password, CREDENTIALS_LENGTH_MAX, stdin);

        response = login(sd, username, password);

        switch (response) {
            case CREDENTIALS_TOO_SHORT:
                printf(" Le credenziali devono essere lunghe almeno " 
                    "%d caratteri\n", CREDENTIALS_LENGTH_MIN - 1);
                break;
            case CREDENTIALS_TOO_LONG:
                printf(" Le credenziali devono essere lunghe al più "
                    "%d caratteri\n", CREDENTIALS_LENGTH_MAX - 1);
                break;
            case LOGIN_FAIL:
                printf(" Password sbagliata, se non hai un account"
                    " utilizza un nome utente diverso\n");
                break;
            case ALREADY_LOGGED_IN:
                printf(" L'account specificato è già in uso su un altro client\n");
                break;
            case LOGIN_SUCCESS:
                printf(" Accesso effettuato con successo\n");
                break;
            case REGISTERED:
                printf(" Account creato correttamente\n");
                break;
            case SERVER_FULL:
                printf(" Il server è pieno, riprovare più tardi\n");
                exit(0);
            default:
                printf(ANSI_COLOR_RED " [Errore]: Connessione interrotta\n" ANSI_COLOR_RESET);
                exit(-1);
        }

    } while(response != LOGIN_SUCCESS && response != REGISTERED);

    printf("\n Benvenuto!\n\n");

    /* Ricezione delle escape room disponibili */
    {
        char *aux_argv[ARGC_MAX];
        int ret, i, aux_argc;

        ret = recv_msg(sd, NULL, &aux_argc, aux_argv);
        if (ret == -1) {
            printf(ANSI_COLOR_RED " [Errore]: Connessione interrotta\n" ANSI_COLOR_RESET);
            exit(-1);
        }

        if (aux_argc == 0) {
            printf(" Al momento non ci sono escape room disponibili\n");
            exit(0);
        }

        printf(" Le escape room disponibili sono:\n");
        for (i = 0; i < aux_argc; i++) {
            printf(" %d) %s\n", i, aux_argv[i]);
        }
        printf("\n");

        free_argv(aux_argv);
    }

    printf(
        " Seleziona l'escape room che vuoi giocare tramite "
            "il comando > start numero\n"
        " In ogni momento puoi stampare una lista dei comandi"
            " disponibili scrivendo > help\n"
    );

    while(1) {

        enum ACTION action;
        int ret;

        printf("\n > ");

        /* Lettura del comando ed invio al server */
        {
            char *aux_argv[ARGC_MAX];
            int aux_argc;

            parse_command(&action, &aux_argc, aux_argv, ARGC_CLIENT_MAX);
            if (action == HELP) {
                /* Il client vuole ricevere aiuto su un comando preciso */
                if (aux_argc >= 1) {
                    action = str_to_action(aux_argv[0]);
                }
                print_help(action);
                free_argv(aux_argv);
                continue;
            }
            
            /* Sintassi errata, per i seguenti comandi è necessario avere almeno un parametro */
            if (aux_argc < 1 &&
                (action == START || action == TAKE || action == USE || action == DROP)) {
                print_help(action);
                free_argv(aux_argv);
                continue;
            }
            
            ret = send_msg(sd, action, aux_argc, aux_argv);
            if (ret == -1) {
                printf(ANSI_COLOR_RED " [Errore]: Connessione interrotta\n" ANSI_COLOR_RESET);
                exit(-1);
            }

            free_argv(aux_argv);
        }
        
        if (action == END) {
            printf(" A presto!\n\n");
            exit(0); 
        }

        /* Lettura della risposta del server al comando */
        {
            char *aux_argv[ARGC_MAX];
            int aux_argc;

            ret = recv_msg(sd, &action, &aux_argc, aux_argv);
            if (ret == -1 || aux_argc <= 0 || 
                (action != QUESTION && action != SERVER)) {
                printf(ANSI_COLOR_RED " [Errore]: Connessione interrotta\n" ANSI_COLOR_RESET);
                exit(-1);
            }

            printf(" %s\n", aux_argv[0]);
            free_argv(aux_argv);
        }

        /* Il server ci ha posto una domanda */
        if (action == QUESTION) {

            char buffer[IO_BUFFER_SIZE];
            char *aux_argv[ARGC_MAX];

            /* Lettura della risposta dallo stdin ed invio di questa al server */
            printf(" Risposta: ");
            fgetsnn(buffer, IO_BUFFER_SIZE, stdin);
            aux_argv[0] = buffer;

            ret = send_msg(sd, ANSWER, 1, aux_argv);
            if (ret == -1) {
                printf(ANSI_COLOR_RED " [Errore]: Connessione interrotta\n" ANSI_COLOR_RESET);
                exit(-1);
            }

            /* Stampa della risposta del server */
            {
                char *aux_argv[ARGC_MAX];
                int ret, aux_argc;

                ret = recv_msg(sd, NULL, &aux_argc, aux_argv);
                if (ret == -1) {
                    printf(ANSI_COLOR_RED " [Errore]: Connessione interrotta\n" ANSI_COLOR_RESET);
                    exit(-1);
                }

                printf(" %s\n", aux_argv[0]);
                free_argv(aux_argv);
            }
        }

    }

    return 0;
}
