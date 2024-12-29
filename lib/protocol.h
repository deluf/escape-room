
#ifndef LIB_PROTOCOL_H
#define LIB_PROTOCOL_H

#define IO_BUFFER_SIZE 1024

/* Tilde, usata per separare gli argomenti nella codifica e decodifica dei messaggi */
#define SEPARATOR 0x7E  

/* Includono '\\0' */
#define CREDENTIALS_LENGTH_MAX 32   
#define CREDENTIALS_LENGTH_MIN 2    

/* Massimo numero di parametri che si possono codificare in un unico messaggio */
#define ARGC_MAX 10

enum ACTION {
    SERVER,     /* Generico messaggio del server */
    CLIENT,     /* Generico messaggio del client */
    HELP,       /* Serve solo al client per controllare la sintassi dei comandi */

    QUESTION,   /* Il server pone una domanda al client e si aspetta una risposta */
    ANSWER,     /* Il client risponde alla domanda precedentemente posta */

    START,      /* Inizio dei comandi del client */    
    LOOK,   
    TAKE,   
    USE,  
    OBJS,
    DROP,
    END,        /* Fine dei comandi del client */

    ACTION_MAX  /* Per i controlli nella decode_messsage(...) */
};

/* Converte gli elementi letterali del tipo ACTION in stringhe */
extern const char* const action_to_str[];

/**
 * Prova a tardurre *buffer* in uno tra:
 *  START, LOOK, TAKE, USE, OBJS, DROP, END.
 * Se non ci riesce ritorna HELP.
 */ 
enum ACTION str_to_action(char *buffer);

enum RESPONSE {
    /* Il database non è neanche stato controllato perchè le credenziali sono troppo corte */
    CREDENTIALS_TOO_SHORT, 
    /* Il database non è neanche stato controllato perchè le credenziali sono troppo lunghe */ 
    CREDENTIALS_TOO_LONG, 
    /* L'username esiste ma la password è sbagliata */  
    LOGIN_FAIL,             
    /* L'username esiste e la password è corretta, ma un'altro client sta giù usando il profilo */
    ALREADY_LOGGED_IN,  
    /* L'username esiste e la password è corretta */    
    LOGIN_SUCCESS,      
    /* L'username non esiste, l'utente è stato registrato */    
    REGISTERED,   
    /* L'username non esiste, ma il server ha esaurito la memoria per registrarlo */          
    SERVER_FULL             
};

/* Converte gli elementi letterali del tipo RESPONSE in stringhe */
extern const char* const response_to_str[];

/**
 * Codifica in *buffer* gli *argc* parametri ricevuti in *argv*.
 * 
 * Scrivere *size* byte in *buffer* senza aver raggiunto il
 *  carattere '\\0' dell'ultimo *argv* viene considerato errore.
 * 
 * In caso di errore ritorna -1, altrimenti il numero di byte scritti in buffer.
 */
int encode_message(char *buffer, int size, enum ACTION action, int argc, char *argv[]);

/**
 * Decodifica il messaggio presente nel *buffer*, inserendo gli argomenti
 *  trovati in *argv* ed il loro numero in *argc* (andranno deallocati
 *  con free_argv(...) dopo averli utilizzati). Modifica anche *action* con
 *  l'azione trovata nel primo byte del *buffer*.
 * 
 * Se *action* non interessa può essere lasciato NULL.
 * E' garantito che tutti gli argv siano terminati da '\\0'.
 * 
 * In caso di errore ritorna -1, altrimenti 0;
 */
int decode_message(const char *buffer, int size, enum ACTION *action, int *argc, char *argv[ARGC_MAX]);

/* Rilascia la memoria occupata dagli *argv* passati come argomento */ 
void free_argv(char *argv[ARGC_MAX]);

/**
 * Tutti i messaggi non banali** vengono scambiati tramite il seguente protocollo:
 *  1. Primo messaggio: dimensione del secondo messaggio su 16 bit.
 *  2. Secondo messaggio:
 *   - Primo byte: *action* (tipo enumerazione, troncato ad 8 bit)
 *   - A partire dal secondo byte: 
 *      *argv[0]*SEPARATOR
 *      *argv[...]*SEPARATOR
 *      *argv[argc-1]*\\0
 * 
 * Se *argc* è 0 non scrive '\\0' alla fine di *buffer* (scrive solo il byte di *action*)
 * 
 * **Un messaggio banale è un messaggio che comporta il solo scambio di un numero intero,
 *  ad esempio la risposta del server ad un tentativo di login, che viene semplicemente
 *  codificata dal tipo enumerazione RESPONSE.
 */ 

/**
 * Invia un messaggio sul socket *sd* seguendo il protocollo descritto sopra.
 * In caso di errore ritorna -1, 0 altrimenti.
 */ 
int send_msg(int sd, enum ACTION action, int argc, char *argv[ARGC_MAX]);

/**
 * Riceve un messaggio sul socket *sd* seguendo il protocollo descritto sopra.
 * Alloca i parametri ricevuti in *argv* e scrive il loro numero in *argc*,
 *  andranno deallocati con free_argv(...) dopo l'utilizzo.
 * 
 * Se *action* non interessa può essere lasciato NULL.
 * E' garantito che tutti gli argv siano terminati da '\\0'.
 * 
 * In caso di errore ritorna -1 (e non è necessario deallocare *argv*), 0 altrimenti.
 */
int recv_msg(int sd, enum ACTION *action, int *argc, char *argv[ARGC_MAX]);

#endif
