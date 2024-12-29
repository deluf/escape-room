
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "lib/protocol.h"
#include "lib/mystdlib.h"
#include "lib/server/database.h"
#include "lib/server/session.h"
#include "lib/server/rooms.h"

#define SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 4242
#define QUEUE_LENGTH 64

/**
 * Stampa l'orario attuale nel formato "[HH:MM:SS.ssssss] > "
 */
void print_current_time(void) {
    struct timeval tv;
    struct tm *tm_info;
    /* 16 è una dimensione sufficiente per contenere HH:MM:SS\\0 */
    char buffer[16];

    gettimeofday(&tv, 0);
    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 9, "%H:%M:%S", tm_info);
    printf(" [%s.%06ld] > ", buffer, tv.tv_usec);
}

/**
 * Se *username* esiste già nel database allora controlla che la password
 *  fornita combaci con quella esistente, altrimenti, se il database non
 *  è pieno, aggiunge un nuovo record.
 */
enum RESPONSE db_check(const char *username, const char *password) {

    enum DB_RESPONSE db_response;
    int username_len, password_len;

    username_len = ssstrlen(username, CREDENTIALS_LENGTH_MAX);
    password_len = ssstrlen(password, CREDENTIALS_LENGTH_MAX);

    if (username_len == -1 || password_len == -1) {
        return CREDENTIALS_TOO_LONG;
    }

    if (username_len < CREDENTIALS_LENGTH_MIN - 1 ||
        password_len < CREDENTIALS_LENGTH_MIN - 1) {
        return CREDENTIALS_TOO_SHORT;
    }

    db_response = db_read(username, password);

    switch (db_response) {
        case DB_READ_SUCCESS:
            return LOGIN_SUCCESS;
        case DB_READ_FAIL:
            return LOGIN_FAIL;    
        default: /* DB_USERNAME_DOES_NOT_EXIST */
            break;
    }

    db_response = db_write(username, password);

    switch (db_response) {
        case DB_WRITE_FAIL:
            return SERVER_FULL;
        default: /* DB_WRITE_SUCCESS */
            return REGISTERED;
    }

}

/* Possibili comandi per il server */
enum COMMAND {
    CMD_NONE,
    CMD_START,
    CMD_STOP
};

/**
 * Legge lo standard input interpretandone i caretteri 
 *  come un comando per il server.
 */ 
enum COMMAND parse_command(void) {
    char buffer[IO_BUFFER_SIZE];

    fgetsnn(buffer, IO_BUFFER_SIZE, stdin);
    if (strncmp(buffer, "start", IO_BUFFER_SIZE) == 0) {
        return CMD_START;
    }
    if (strncmp(buffer, "stop", IO_BUFFER_SIZE) == 0) {
        return CMD_STOP;
    }
    return CMD_NONE;
}

/**
 * Aspetta ciclicamente che venga inserito il comando start.
 */
void wait_for_start(void) {
    enum COMMAND command;
    do {
        command = parse_command();
        switch (command) {
            case CMD_NONE:
                printf("\n Comando inesistente\n\n > ");
                break;
            case CMD_START:
                break;
            case CMD_STOP:
                printf("\n Il server non è in esecuzione\n\n > ");
                break;
        }
    }
    while (command != CMD_START);
}

/**
 * Se il comando inserito è quello di stop, e nessun 
 *  client è in gioco, allora termina il server.
 */ 
void stdin_ready(void) {
    enum COMMAND command;

    command = parse_command();
    switch (command) {
        case CMD_NONE:
            print_current_time();
            printf("Comando inesistente\n");
            break;
        case CMD_START:
            print_current_time();
            printf("Il server è già in esecuzione\n");
            break;
        case CMD_STOP:
            if (get_session_by_room(-1) != NULL) {
                print_current_time();
                printf("Impossibile arrestare il server, almeno un client è in gioco\n");
                break;
            }
            printf("\n################################################################################\n\n");
            exit(0);
    }
}

/**
 * Accetta la connessione da parte di un nuovo client.
 * In caso di errore (o disconnessione) ritorna -1, altrimenti
 *  ritorna il socket descriptor di comunicazione col client.
 */ 
int listener_ready(int sd) {

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];
    int client_port, new_sd;

    new_sd = accept(sd, (struct sockaddr*)&client_addr, &client_len);
    if (new_sd == -1) {
        print_current_time();
        printf("Impossibile inizializzare una connessione con %d\n", sd);
        return -1;
    }

    inet_ntop(AF_INET, (void *)&client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    client_port = ntohs(client_addr.sin_port);

    print_current_time();
    printf("Client %s:%i connesso con ID %d\n", client_ip, client_port, new_sd);  

    return new_sd;
}

/**
 * Completa la procedura di login con un client e inizializza una sessione.
 * Se è andata a buon fine invia anche la lista delle escape room.
 * In caso di errore (o disconnessione) ritorna -1, altrimenti 0.
 */
int login_and_send_rooms(int sd) {

    enum RESPONSE n_response, h_response;
    struct session *session;
    int argc, ret, i;
    char *argv[ARGC_MAX];
    char username[CREDENTIALS_LENGTH_MAX];

    ret = recv_msg(sd, NULL, &argc, argv);
    /* Voglio esattamente 2 argomenti, argv[0] = username, argv[1] = password */
    if (ret == -1 || argc != 2) {
        print_current_time();
        printf("Connessione con %d interrotta\n", sd);
        return -1;
    }

    /* Controllo nel database e nella lista delle sessioni */
    h_response = db_check(argv[0], argv[1]);
    session = get_session_by_username(argv[0]);
    if (h_response == LOGIN_SUCCESS && session != NULL) {
        h_response = ALREADY_LOGGED_IN;
    }
    n_response = htonl(h_response);

    /* Ricopio l'username, che mi servirà dopo, e dealloco gli *argv* */
    strcpy(username, argv[0]);
    free_argv(argv);

    print_current_time();
    printf("%d ha effettuato un tentativo di login, "
        "con risultato: %s\n", sd, response_to_str[h_response]);
    
    /* Invio della risposta (dimensione nota) */
    ret = send(sd, &n_response, sizeof(n_response), MSG_NOSIGNAL);
    if (ret == -1) {
        print_current_time();
        printf("Connessione con %d interrotta\n", sd);
        return -1;
    }

    /* Casi in cui viene consentito riprovare l'accesso */
    if (h_response == LOGIN_FAIL ||
        h_response == ALREADY_LOGGED_IN ||
        h_response == CREDENTIALS_TOO_SHORT ||
        h_response == CREDENTIALS_TOO_LONG) {
        return 0;
    }

    if (h_response == SERVER_FULL) {
        printf(ANSI_COLOR_YELLOW  "[Warning]: Impossibile creare un nuovo "
            "record per %d, memoria esaurita\n" ANSI_COLOR_RESET, sd);
        return -1;
    }

    /**
     * Il login è andato a buon fine. Crea una sessione che ha come
     *  ID il socket descriptor del client appena autenticato.
     */
    session = init_session(sd, username);
    if (session == NULL) {
        printf(ANSI_COLOR_YELLOW "[Warning]: Impossibile creare una nuova "
            "sessione per %d, memoria esaurita\n" ANSI_COLOR_RESET, sd);
        return -1;
    }

    /* Codifica delle escape room, riutilizza il contenitore *argv* */
    argc = N_ROOMS;
    for (i = 0; i < N_ROOMS; i++) {
        argv[i] = g_rooms[i].name;
    }

    ret = send_msg(sd, SERVER, argc, argv);
    if (ret == -1) {
        print_current_time();
        printf("Connessione con %d interrotta\n", sd);
        return -1;
    }

    print_current_time();
    printf("%d ha ricevuto la lista delle escape room\n", sd);

    return 0;
}

/**
 * Invia il messaggio testuale contenuto in *str* al client. 
 * Appende alla risposta il tempo rimasto ed i token raccolti.
 * Ritorna -1 in caso di errore, 0 altrimenti.
 */
int send_text(int sd, char *str, struct session *session) {
    char buffer[IO_BUFFER_SIZE];
    char *argv[ARGC_MAX];
    unsigned long end_time, remaining_time;

    end_time = session->start_time + g_rooms[session->room].time_limit * 60;
    remaining_time = end_time - (unsigned long)time(NULL);

    sprintf(buffer, "%s\n [Tempo rimasto: %lus, Token raccolti: %d/%d]", 
        str, remaining_time, session->n_tokens, g_rooms[session->room].n_tokens); 

    argv[0] = buffer;
    return send_msg(sd, SERVER, 1, argv);
}

/**
 * Invia il messaggio testuale contenuto in *str* al client.
 * Ritorna -1 in caso di errore, 0 altrimenti.
 */
int send_text_without_info(int sd, enum ACTION action, char *str, struct session *session) {
    char buffer[IO_BUFFER_SIZE];
    char *argv[ARGC_MAX];

    strcpy(buffer, str);
    argv[0] = buffer;
    return send_msg(sd, action, 1, argv);
}

/**
 * Gestisce ricezione, interpretazione e risposta al client
 *  del comando START e dei suoi argomenti.
 * Ritorna -1 in caso di errore.
 */
int start_command(int sd, struct session* session, int argc, char *argv[ARGC_MAX]) {
    int room;
    struct session *s;
    char buffer[IO_BUFFER_SIZE];

    if (argc < 1) {
        strcpy(buffer, "Questo comando necessita di almeno un parametro.");
        return send_text_without_info(sd, SERVER, buffer, session);
    }
    
    /** 
     * atoi ritorna 0 in caso di errore, va differenziato dal caso
     *  in cui è stato effettivamente tradotto il numero 0.
     */
    room = atoi(argv[0]);
    if ((room == 0 && argv[0][0] != '0') ||
        room < 0 || 
        room >= N_ROOMS) {
        strcpy(buffer, "La room inserita non esiste.");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    if (session->room == room) {
        strcpy(buffer, "Sei già in questa stanza.");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    /* Vediamo se prima di far entrare il giocatore nuovo c'era qualcuno */
    s = get_session_by_room(room);

    /** 
     * Servono, se il client prova ad entrare in una stanza occupata, a 
     *  riconoscere dove voleva entrare quando invierà la risposta.
     */
    session->room = room;
    session->answer_to = NULL;

    /* Il client ha provato ad entrare in una room occupata */
    if (s != NULL) {
        print_current_time();
        printf("%d ha provato ad entrare nella room %d, già occupata\n", sd, room);

        strcpy(buffer, "C'è già un giocatore in questa stanza. Se rispondi bene alla seguente domanda gli verrà tolto del tempo, altrimenti gliene verrà aggiunto! ");
        strcat(buffer, g_rooms[room].question);
        return send_text_without_info(sd, QUESTION, buffer, session);
    }
    
    /* Inizializzazione dei restanti campi della sessione, se la stanza era vuota */
    session->n_objects = 0;
    session->n_tokens = 0;
    session->start_time = (unsigned long)time(NULL);
    load_statuses(session, room);
    
    print_current_time();
    printf("%d ha iniziato a giocare nella room %d\n", sd, session->room);
    sprintf(buffer, "Benvenuto nella room %s. Hai %d minuti a partire da ora!",
        g_rooms[room].name, g_rooms[room].time_limit);
    return send_text(sd, buffer, session);
}

/**
 * Gestisce ricezione, interpretazione e risposta al client
 *  del comando LOOK e dei suoi argomenti.
 * Ritorna -1 in caso di errore.
 */
int look_command(int sd, struct session* session, int argc, char *argv[ARGC_MAX]) {   
    char buffer[IO_BUFFER_SIZE];
    struct location *location;
    struct object *object;

    if (session->room == -1) {
        strcpy(buffer, "Attualmente non sei in nessuna stanza");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    if (argc == 0) {
        strcpy(buffer, g_rooms[session->room].look_msg);
        return send_text(sd, buffer, session);
    }

    /* argc >= 1 */
    location = get_location(session->room, argv[0]);
    object = get_object(session->room, argv[0]);

    /* Comando look eseguito su una locazione */
    if (location != NULL) {
        strcpy(buffer, location->look_msg);
    }
    /* Comando look eseguito su un oggetto */
    else if (object != NULL) {
        struct object_status *os = get_status(session, object);
        enum TAKE_STATUS ts = object->take[os->times_taken];

        /* Il client lo ha già sbloccato */
        if (ts == OBJ_UNLOCKED || ts == OBJ_GIVE_TOKEN) {
            strcpy(buffer, object->unlocked_look_msg);
        }
        /* Il client non lo ha ancora sbloccato */
        else {
            strcpy(buffer, object->locked_look_msg);
        }
    }
    /* Comando look eseguito su qualcosa di inesistente */
    else {
        strcpy(buffer, "Non c'è nessuna locazione od oggetto con questo nome.");
    }

    return send_text(sd, buffer, session);
}

/**
 * Gestisce ricezione, interpretazione e risposta al client
 *  del comando TAKE e dei suoi argomenti.
 * Ritorna -1 in caso di errore.
 */
int take_command(int sd, struct session* session, int argc, char *argv[ARGC_MAX]) {
    char buffer[IO_BUFFER_SIZE];
    struct object *object;
    struct object_status *os;
    enum TAKE_STATUS ts;

    if (session->room == -1) {
        strcpy(buffer, "Attualmente non sei in nessuna stanza.");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    if (argc < 1) {
        strcpy(buffer, "Questo comando richiede almeno un parametro.");
        return send_text(sd, buffer, session);
    }

    object = get_object(session->room, argv[0]);
    if (object == NULL) {
        strcpy(buffer, "L'oggetto specificato non esiste.");
        return send_text(sd, buffer, session); 
    }

    os = get_status(session, object);
    if (os->in_inventory) {
        strcpy(buffer, "Hai già questo oggetto in mano.");
        return send_text(sd, buffer, session);   
    }

    if (session->n_objects == OBJECTS_PER_PLAYER_MAX) {
        strcpy(buffer, "Hai troppi oggetti in mano, devi posarne qualcuno.");
        return send_text(sd, buffer, session);   
    }

    ts = object->take[os->times_taken];
    if (ts == OBJ_UNLOCKED) {
        strcpy(buffer, "Oggetto raccolto.");
        session->n_objects++;
        os->in_inventory = 1;
    }
    else if (ts == OBJ_GIVE_TOKEN) {
        session->n_tokens++;
        session->n_objects++;
        os->in_inventory = 1;
        os->times_taken++;

        if (g_rooms[session->room].n_tokens == session->n_tokens) {
            print_current_time();
            printf("%d ha risolto la room %d\n", sd, session->room);

            session->room = -1;
            strcpy(buffer, "Hai raccolto tutti i token in tempo! Bel lavoro.");
            return send_text_without_info(sd, SERVER, buffer, session);
        }
        else {
            strcpy(buffer, "Oggetto raccolto. Ti è stato assegnato un token!");
        }
    }
    else if (ts == OBJ_LOCKED_BY_Q) {
        session->answer_to = object;
        strcpy(buffer, "L'oggetto è bloccato da un enigma:\n ");
        strcat(buffer, object->take_q);
        return send_text_without_info(sd, QUESTION, buffer, session);
    }
    /* OBJ_LOCKED_BY_USE */
    else {
        strcpy(buffer, "L'oggetto è bloccato...");
    }

    return send_text(sd, buffer, session);
}

/**
 * Gestisce ricezione, interpretazione e risposta al client
 *  del comando USE e dei suoi argomenti.
 * Ritorna -1 in caso di errore.
 */
int use_command(int sd, struct session* session, int argc, char *argv[ARGC_MAX]) {
    char buffer[IO_BUFFER_SIZE];
    struct object *object1, *object2;
    struct object_status *os1, *os2;

    if (session->room == -1) {
        strcpy(buffer, "Attualmente non sei in nessuna stanza");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    if (argc < 1) {
        strcpy(buffer, "Questo comando richiede almeno un parametro.");
        return send_text(sd, buffer, session);
    }
    
    object1 = get_object(session->room, argv[0]);
    if (object1 == NULL) {
        strcpy(buffer, "Il primo oggetto specificato non esiste.");
        return send_text(sd, buffer, session); 
    }
    os1 = get_status(session, object1);

    if (os1->used) {
        strcpy(buffer, "Hai già usato questo oggetto.");
        return send_text(sd, buffer, session); 
    }

    if (!os1->in_inventory) {
        strcpy(buffer, "Devi avere l'oggetto in mano per poterlo utilizzare.");
        return send_text(sd, buffer, session); 
    }

    /* L'oggetto deve essere utilizzato da solo */
    if (object1->use_with == NULL) {
        /* Viene effettivamente usato da solo */
        if (argc == 1) {
            os1->used = 1;
            strcpy(buffer, object1->use_msg);
        }
        /* Viene utilizzato con un altro oggetto */
        else {
            strcpy(buffer, "Non sembra fare nulla.");
        }
    }
    /* L'oggetto deve essere utilizzato con un'altro */
    else {
        if (argc < 2) {
            strcpy(buffer, "Non sembra fare nulla.");
            return send_text(sd, buffer, session); 
        }
        
        object2 = get_object(session->room, argv[1]);
        if (object2 == NULL) {
            strcpy(buffer, "Il secondo oggetto specificato non esiste.");
            return send_text(sd, buffer, session); 
        }

        if (object1->use_with != object2) {
            strcpy(buffer, "Non sembra fare nulla.");
        }
        else {
            os1->used = 1;
            os2 = get_status(session, object2);
            os2->times_taken++;
            strcpy(buffer, object1->use_msg);
        }
    }

    return send_text(sd, buffer, session); 
}

/**
 * Gestisce ricezione, interpretazione e risposta al client
 *  del comando OBJS e dei suoi argomenti.
 * Ritorna -1 in caso di errore.
 */
int objs_command(int sd, struct session* session, int argc, char *argv[ARGC_MAX]) {
    char buffer[IO_BUFFER_SIZE];

    if (session->room == -1) {
        strcpy(buffer, "Attualmente non sei in nessuna stanza");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    if (session->n_objects == 0) {
        strcpy(buffer, "Non hai nessun oggetto.");
    }
    else {
        int i;
        strcpy(buffer, "");
        for (i = 0; i < g_rooms[session->room].tot_objects; i++) {
            if (session->objects_statuses[i].in_inventory) {
                strcat(buffer, session->objects_statuses[i].object->name);
                strcat(buffer, "\n ");
            }
        }
        /* Rimuove l'ultimo '\\n ' */
        buffer[strlen(buffer) - 2] = '\0';
    }

    return send_text(sd, buffer, session);
}

/**
 * Gestisce ricezione, interpretazione e risposta al client
 *  del comando DROP e dei suoi argomenti.
 * Ritorna -1 in caso di errore.
 */
int drop_command(int sd, struct session* session, int argc, char *argv[ARGC_MAX]) {
    char buffer[IO_BUFFER_SIZE];
    struct object *object;
    struct object_status *os;

    if (session->room == -1) {
        strcpy(buffer, "Attualmente non sei in nessuna stanza");
        return send_text_without_info(sd, SERVER, buffer, session);
    }

    if (argc < 1) {
        strcpy(buffer, "Questo comando richiede almeno un parametro.");
        return send_text(sd, buffer, session);
    }

    object = get_object(session->room, argv[0]);
    if (object == NULL) {
        strcpy(buffer, "L'oggetto specificato non esiste.");
        return send_text(sd, buffer, session);
    }

    os = get_status(session, object);
    if (!os->in_inventory) {
        strcpy(buffer, "Puoi posare solamente oggetti che hai in mano.");
    }
    else {
        os->in_inventory = 0;
        session->n_objects--;
        strcpy(buffer, "Oggetto posato.");       
    }

    return send_text(sd, buffer, session);
}

/**
 * Gestisce la ricezione delle risposte del client agli enigmi.
 * Ritorna -1 in caso di errore, 0 altrimenti
 */
int handle_answers(int sd, struct session *session, int argc, char *argv[ARGC_MAX]) {
    char buffer[IO_BUFFER_SIZE];

    if (argc <= 0 || session->room == -1) {
        print_current_time();
        printf("Connessione con %d interrotta\n", sd);
        return -1;
    }

    /* Il client sta rispondedo all'enigma per entrare in una room occupata */
    if (session->answer_to == NULL) {
        
        struct session *s;
        int room;

        /** 
         * Resetta la room del giocatore che risponde alla domanda
         *  (così get_session_by_room(...) ritorna quella dell'altro).
         */
        room = session->room;
        session->room = -1;

        /* Recupera la sessione del giocatore attualmente in gioco in tale stanza */
        s = get_session_by_room(room);
        if (s == NULL) {
            strcpy(buffer, "Il giocatore è uscito dalla stanza prima che tu rispondessi.");
            
            print_current_time();
            printf("%d ha risposto alla domanda ma il giocatore precedente è già uscito\n", sd);
        }
        else if (strcmp(argv[0], g_rooms[room].answer) == 0) {
            sprintf(buffer, "Risposta corretta! Sono stati tolti %d"
                " minuti a %s.", g_rooms[room].bonus, s->username);
            s->start_time -= g_rooms[room].bonus * 60;
            
            print_current_time();
            printf("%d ha risposto correttamente alla domanda, danneggiando %d\n", sd, s->sd);
        }
        else {
            sprintf(buffer, "Risposta sbagliata! Sono stati aggiunti %d"
                " minuti %s.", g_rooms[room].penalty, s->username);
            s->start_time += g_rooms[room].penalty * 60;

            print_current_time();
            printf("%d ha risposto in modo errato alla domanda, avvantaggiando %d\n", sd, s->sd);
        }

    }
    
    /* Il client sta rispondendo ad un enigma per sbloccare un oggetto */
    else {
        if (strcmp(argv[0], session->answer_to->take_a) == 0) {
            struct object_status *os = get_status(session, session->answer_to);
            os->times_taken++;
            strcpy(buffer, "Risposta corretta! Adesso puoi raccogliere l'oggetto.");

            print_current_time();
            printf("%d ha risposto correttamente ad un enigma\n", sd);
        }
        else {
            strcpy(buffer, "Risposta sbagliata.");

            print_current_time();
            printf("%d ha risposto in modo errato ad un enigma\n", sd);
        }
    }

    return send_text_without_info(sd, SERVER, buffer, session);
}

/**
 * Gestisce i comandi di gioco ricevuti dal client.
 * In caso di errore (o disconnessione) ritorna -1, altrimenti 0.
 */
int play(int sd, struct session *session) {

    int ret, argc, i;
    enum ACTION action;
    char buffer[IO_BUFFER_SIZE];
    char *argv[ARGC_MAX];

    /* Ricezione del messaggio */
    ret = recv_msg(sd, &action, &argc, argv);
    if (ret == -1 || action < ANSWER || action > END) { 
        printf(ANSI_COLOR_YELLOW "[Warning]: impossibile decodificare il messaggio "
            "ricevuto da %d. Connessione terminata\n" ANSI_COLOR_RESET, sd);
        free_argv(argv);
        return -1;
    }

    print_current_time();
    printf("%d ha inviato il comando %s", sd, action_to_str[action]); 
    for (i = 0; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");

    if (action == ANSWER) {
        ret = handle_answers(sd, session, argc, argv);
        free_argv(argv);
        return ret;
    }

    if (action == END) {
        print_current_time();
        printf("Connessione con %d interrotta\n", sd);
        free_argv(argv);
        return -1;
    }

    /**
     * Se il client è già in gioco, ed il comando che invia non è START, 
     *  controlla se ha finito il tempo, in tal caso notifica il client
     *  che ha perso e termina la partita.
     */
    if (action != START && session->room != -1) {
        int elapsed_time = (int)difftime(time(NULL), session->start_time);
        if (elapsed_time > g_rooms[session->room].time_limit * 60) {
            print_current_time();
            printf("%d ha esaurito il tempo nella room %d\n", sd, session->room);
            session->room = -1;
            free_argv(argv);
            strcpy(buffer, "Il tempo è scaduto, hai perso!");
            return send_text_without_info(sd, SERVER, buffer, session);
        }
    }

    /**
     * A questo punto *action* contiene il comando scritto da client e
     *  *argc* il numero di argomenti per il comando (memorizzati in *argv).
     */
    switch (action) {
        case START:
            ret = start_command(sd, session, argc, argv);
            break;
        case LOOK:
            ret = look_command(sd, session, argc, argv);
            break;
        case TAKE:
            ret = take_command(sd, session, argc, argv);
            break;
        case USE:
            ret = use_command(sd, session, argc, argv);
            break;
        case OBJS:
            ret = objs_command(sd, session, argc, argv);
            break;
        case DROP:
            ret = drop_command(sd, session, argc, argv);
            break;
        default:    /* Mai utilizzato (controlliamo action prima) */
            break;
    }

    #ifdef MDEBUG
    if (action != START) {
        printf("\t#Stato degli oggetti del giocatore %d\n", sd);
        for (i = 0; i < g_rooms[session->room].tot_objects; i++) {
            printf("\t%-15s in_inventory: %d used: %d times_taken: %d\n", session->objects_statuses[i].object->name, session->objects_statuses[i].in_inventory, session->objects_statuses[i].used, session->objects_statuses[i].times_taken);
        }
    }
    #endif

    free_argv(argv);

    if (ret == -1) {
        printf("Impossibile eseguire il comando ricevuto da %d. Connessione terminata\n", sd);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {

    int server_port, listener, ret, sd_max;
    struct sockaddr_in server_addr;    
    fd_set master_read, read_fds;

    printf("\n############################## INTERFACCIA SERVER ##############################\n\n");

    /* Controllo degli argomenti passati da riga di comando */
    if (argc >= 2) {
        server_port = atoi(argv[1]);
        if (server_port <= 0 || server_port > 65535) {
            printf(" Il numero di porta deve essere compreso tra 1 e 65535\n\n");
            printf("################################################################################\n\n");
            exit(-1);
        }
        printf(" E' stata scelta la porta %d\n\n", server_port);
    }
    else {
        printf(" Nessuna porta specificata, verrà usata la porta di default %d\n\n", DEFAULT_SERVER_PORT);
        server_port = DEFAULT_SERVER_PORT;
    }

    printf(
        " Comandi disponibili:\n"
        " > start\t# Avvia il server\n"
        " > stop \t# Termina il server\n"
        "\n"
        " > "
    );

    wait_for_start();

    printf("\n Puoi fermare il server quando vuoi tramite il comando stop\n\n");
    printf("################################################################################\n\n");

    /* Creazione del socket */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror_fatal();
        exit(-1);
    } 

    /* Configurazione del socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Applicazione della configurazione al socket */
    ret = bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror_fatal();
        exit(-1);
    } 

    /* Definizione del socket come passivo */
    ret = listen(listener, QUEUE_LENGTH);
    if (ret == -1) {
        perror_fatal();
        exit(-1);
    }

    /**
     * IO multiplexing tramite primitiva select per gestire
     * le richieste dei client e lo stdin (comando stop).
     */
    FD_ZERO(&master_read);
    FD_ZERO(&read_fds);
    FD_SET(listener, &master_read);
    FD_SET(STDIN_FILENO, &master_read);

    /* STDIN_FILENO è sempre 0, quindi listener è il maggiore */
    sd_max = listener; 

    if (init_rooms() == -1) {
        printf(ANSI_COLOR_RED "[Errore]: impossibile caricare "
            "in memoria la escape room\n" ANSI_COLOR_RESET);
        exit(-1);
    }

    print_current_time();
    printf("Server in ascolto su %s:%i\n", SERVER_IP, server_port);

    while(1) {

        int sd;
        read_fds = master_read;

        /* No timeout (il server non deve effettuare operazioni asincrone) */
        select(sd_max + 1, &read_fds, NULL, NULL, NULL);

        for (sd = 0; sd <= sd_max; sd++) {

            /* Socket inesistente o non pronto per la lettura */
            if (!FD_ISSET(sd, &read_fds)) {
                continue;
            }

            /* Sono stati scritti dei byte sullo standard input */
            if (sd == STDIN_FILENO) {
                stdin_ready();
            }

            /* Sono stati scritti dei byte nel socket di connessione */
            else if (sd == listener) {

                /**
                 * In caso di errore nell'accettare la connessione di un particolare 
                 *  client il server prova lo stesso a continuare l'esecuzione
                 */
                int new_sd = listener_ready(sd);
                if (new_sd == -1) {
                    continue;
                }

                FD_SET(new_sd, &master_read);
                if (new_sd > sd_max) sd_max = new_sd;
            }

            /* Sono stati scritti dei byte su un socket di comunicazione */
            else {
                struct session *session;
                int ret;

                /* Recupera la sessione del client */
                session = get_session_by_sd(sd);
                
                if (session == NULL) {
                    ret = login_and_send_rooms(sd);
                }
                else {
                    ret = play(sd ,session);
                }

                /* In caso di errore chiudi la connessione e la sessione */
                if (ret == -1) {
                    FD_CLR(sd, &master_read);
                    close(sd);
                    close_session(sd);
                }
            }

        }
    
    }
    
    return 0;
}
