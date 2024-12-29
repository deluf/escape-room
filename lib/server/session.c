
#include <string.h>
#include <stdlib.h>

#include "session.h"

struct session *g_sessions = NULL;

struct session* init_session(int sd, const char *username) {
    struct session *s;

    s = malloc(sizeof(struct session));
    if (s == NULL) {
        return NULL;
    }

    s->sd = sd;
    s->room = -1;
    strcpy(s->username, username); 

    s->next = g_sessions;
    g_sessions = s;

    return s;
}

void close_session(int sd) {
    struct session *old;
    struct session **s = &g_sessions;

    while((*s) != NULL && (*s)->sd != sd) {
        s = &(*s)->next;
    }

    /* Elemento non trovato */
    if ((*s) == NULL) {
        return;
    }

    old = *s;
    *s = old->next;
    free(old);
}

void load_statuses(struct session *session, int room) {
    int i, j, k;

    memset(session->objects_statuses, 0, sizeof(session->objects_statuses));
    
    k = 0;
    for (i = 0; i < g_rooms[room].n_locations; i++) {
        for (j = 0; j < g_rooms[room].locations[i].n_objects; j++) {
            session->objects_statuses[k].object = &g_rooms[room].locations[i].objects[j];
            k++;
        }
    }
}

struct object_status* get_status(struct session *session, struct object *obj) {
    int i = 0;
    while (i < g_rooms[session->room].tot_objects &&
           session->objects_statuses[i].object != obj) {
        i++;
    }
    return &session->objects_statuses[i];
}

struct session* get_session_by_room(int room) {
    struct session *s = g_sessions;
    while (s != NULL) {
        if ((room == -1 && s->room != -1) || 
            (room != -1 && s->room == room)) {
            return s;
        }
        s = s->next;
    }
    return s;
}

struct session* get_session_by_sd(int sd) {
    struct session *s = g_sessions;
    while (s != NULL && s->sd != sd) {
        s = s->next;
    }
    return s;
}

struct session* get_session_by_username(const char *username) {
    struct session *s = g_sessions;
    while (s != NULL && strcmp(s->username, username) != 0) {
        s = s->next;
    }
    return s;
}
