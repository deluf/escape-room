
#ifndef LIB_SERVER_SESSION_H
#define LIB_SERVER_SESSION_H

#include "../protocol.h"
#include "rooms.h"

struct object_status {
    struct object *object;
    int used;           /* 0 o 1, indica se l'oggetto è stato utilizzato o meno */
    int in_inventory;   /* 0 o 1, indica se l'oggetto è attualmente nell'inventario o meno */

    /* 0, 1 o 2, indica quale TAKE_STATUS guardare quando si prova a raccogliere l'oggetto */
    int times_taken;
};

struct session {
    int sd;
    char username[CREDENTIALS_LENGTH_MAX];

    /* L'escape room in cui sta attualmente giocando, -1 se non sta giocando */
    int room;

    /* Numero di oggetti e di token attualmente posseduti */
    int n_objects, n_tokens; 

    unsigned long start_time;  /* Unix timestamp */

    /**
     * Serve per discriminare a quale enigma di quale oggetto sta rispondendo 
     *  il client, vale NULL se sta rispondendo alla domanda per entrare in una
     *  room occupata.
     */
    struct object *answer_to;

    /* Memorizza lo stato di tutti gli oggetti di una stanza */
    struct object_status objects_statuses[OBJECTS_PER_LOCATION_MAX * OBJECTS_PER_PLAYER_MAX];

    struct session *next;
};

/**
 * Inizializza una sessione per un nuovo client.
 * In caso di memoria piena o di identificatore già in uso ritorna NULL.
 */
struct session* init_session(int sd, const char *username);

/**
 * Termina una sessione e libera la memoria occupata da quest'ultima.
 * Se *sd* non è un identificatore di sessione valido non fa nulla.
 */
void close_session(int sd);

/**
 * Carica in *session* gli indirizzi degli oggetti nella
 *  stanza *room* e ne inizializza i valori di controllo.
 */
void load_statuses(struct session *session, int room);

/**
 * Ritorna lo stato dell'oggetto *obj* per il client con sessione *session*
 */
struct object_status* get_status(struct session *session, struct object *obj);

/**
 * Se almeno un giocatore sta giocando la stanza *room* ritorna
 *  un puntatore alla sessione del più recente, NULL altrimenti.
 * Se *room* == -1 la ricerca è globale (praticamente ritorna
 *  qualcosa != NULL se almeno un giocatore sta giocando a
 *  qualsiasi stanza)
 */ 
struct session* get_session_by_room(int room);

struct session* get_session_by_sd(int sd);
struct session* get_session_by_username(const char *username);

#endif
