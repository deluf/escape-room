
#ifndef ROOMS_H
#define ROOMS_H

#include "../protocol.h"

#define N_ROOMS 1
#define LOCATIONS_MAX 3
#define OBJECTS_PER_LOCATION_MAX 3
#define OBJECTS_PER_PLAYER_MAX 3

extern struct room *g_rooms;

enum TAKE_STATUS {
    OBJ_GIVE_TOKEN,         /* Quando l'oggetto viene preso rilascia un token */
    OBJ_UNLOCKED,           /* L'oggetto non è bloccato da niente */
    OBJ_LOCKED_BY_USE,      /* L'oggetto è bloccato dall'utilizzo di un altro oggetto */
    OBJ_LOCKED_BY_Q         /* L'oggetto è bloccato da una domanda */
};

struct object {
    /* Definiscono cosa succede quando si prova a prende l'oggetto */
    enum TAKE_STATUS take[3];

    /* NULL se va usato da solo, altrimenti un puntatore all'altro oggetto */
    struct object *use_with;

    char *name;

    /* Il messaggio di look può cambiare a seconda dello stato dell'oggetto */
    char *locked_look_msg;
    char *unlocked_look_msg;

    /* Il messaggio che l'utente riceve se utilizza l'oggetto come dovrebbe */
    char *use_msg;

    /* Se l'oggetto è OBJ_LOCKED_BY_Q pone la domanda take_q e aspetta la risposta take_a */
    char *take_q;
    char *take_a;
};

struct location {
    int n_objects;
    char *name;
    char *look_msg;
    struct object objects[OBJECTS_PER_LOCATION_MAX];
};

struct room {
    int id, n_locations, n_tokens, tot_objects;
    char *name;
    char *look_msg;

    /**
     * La domanda che verrà fatta ai giocatori che provano
     *  a connettersi quando ne è già in gioco uno.
     */
    char *question; 

    /* La risposta a tale domanda*/
    char *answer;

    /**
     * Tutti i tempi sono espressi in minuti. *penalty* e *bonus*
     *  sono i tempi che vengono tolti/aggiunti quando l'utente
     *  sbaglia o indovina la domanda descritta sopra.
     */
    int time_limit, penalty, bonus;

    struct location locations[LOCATIONS_MAX];
};

/**
 * Carica nella variabile globale *g_rooms* le escape room.
 * Ritorna -1 in caso di errore.
 */
int init_rooms(void);

struct location* get_location(int room, const char *name);
struct object* get_object(int room, const char *name);

#endif
