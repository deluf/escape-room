
#include <string.h> 
#include <stdlib.h> 
#include <stdio.h> 

#include "rooms.h"

struct room *g_rooms;

int init_rooms(void) {

    int size = sizeof(struct room) * N_ROOMS;

    g_rooms = malloc(size);
    if (g_rooms == NULL) {
        return -1;
    }
    memset(g_rooms, 0, size);

    /* 0) Red Teaming */
    g_rooms[0].id = 0;
    g_rooms[0].time_limit = 10;
    g_rooms[0].penalty = 3;
    g_rooms[0].bonus = 3;
    g_rooms[0].name = "Red Teaming";
    g_rooms[0].look_msg = "Sei parte di un gruppo di hacker in una missione di red teaming, siete appena entrati nell'edificio target. Ti trovi in uno degli uffici al secondo piano. Alla tua destra c'è una ++scrivania++ di legno con sopra un **computer** ed un **router**. Vicino all'ingresso c'è una ++scatola++ di cartone con dentro un **cavo** ed una **tastiera**. Dietro di te c'è una ++libreria++. Il tuo obbiettivo è quello di sbloccare il computer e connetterlo ad internet, i tuoi compagni si occuperanno del resto.";
    g_rooms[0].question = "Come si chiama quel software o dispositivo hardware che osserva e filtra i pacchetti in ingresso o uscita da una rete?\n  a) Antivirus\n  b) Firewall\n  c) Cookie\n  d) Router";
    g_rooms[0].answer = "b"; 
    g_rooms[0].n_locations = 3;
    g_rooms[0].n_tokens = 3;
    g_rooms[0].tot_objects = 6;
    {
        /* scrivania */
        g_rooms[0].locations[0].name = "scrivania";
        g_rooms[0].locations[0].look_msg = "Si tratta di una moderna scrivania di legno con sopra un **computer** ed un **router**.";
        {
            g_rooms[0].locations[0].n_objects = 3;
            {
                /* computer */
                g_rooms[0].locations[0].objects[0].name = "computer";
                g_rooms[0].locations[0].objects[0].locked_look_msg = "Non ho niente con cui scrivere...";
                g_rooms[0].locations[0].objects[0].unlocked_look_msg = "Mi chiede una **password** per entrare.";
                g_rooms[0].locations[0].objects[0].take[0] = OBJ_LOCKED_BY_USE;
                g_rooms[0].locations[0].objects[0].take[1] = OBJ_UNLOCKED;
                g_rooms[0].locations[0].objects[0].use_with = NULL;
                g_rooms[0].locations[0].objects[0].use_msg = "Non sembra fare nulla.";

                /* router */
                g_rooms[0].locations[0].objects[1].name = "router";
                g_rooms[0].locations[0].objects[1].locked_look_msg = "Tutte le luci sono rosse, non è connesso ad internet. Sembra che manchi qualcosa...";
                g_rooms[0].locations[0].objects[1].unlocked_look_msg = "Le luci sono diventate verdi, è connesso!";
                g_rooms[0].locations[0].objects[1].take[0] = OBJ_LOCKED_BY_USE;
                g_rooms[0].locations[0].objects[1].take[1] = OBJ_GIVE_TOKEN;
                g_rooms[0].locations[0].objects[1].take[2] = OBJ_UNLOCKED;
                g_rooms[0].locations[0].objects[1].use_with = NULL;
                g_rooms[0].locations[0].objects[1].use_msg = "Non sembra fare nulla.";

                /* password */
                g_rooms[0].locations[0].objects[2].name = "password";
                g_rooms[0].locations[0].objects[2].locked_look_msg = "Non c'è molto da osservare...";
                g_rooms[0].locations[0].objects[2].unlocked_look_msg = "Hai completato il tuo obbiettivo in tempo. Bel lavoro!";
                g_rooms[0].locations[0].objects[2].take[0] = OBJ_LOCKED_BY_Q;
                g_rooms[0].locations[0].objects[2].take[1] = OBJ_GIVE_TOKEN;
                g_rooms[0].locations[0].objects[2].take[2] = OBJ_UNLOCKED;
                g_rooms[0].locations[0].objects[2].take_q = "La password è un codice numerico di 6 cifre...";
                g_rooms[0].locations[0].objects[2].take_a = "250513";
                g_rooms[0].locations[0].objects[2].use_with = NULL;
                g_rooms[0].locations[0].objects[2].use_msg = "Non sembra fare nulla.";
            }
        }

        /* scatola */
        g_rooms[0].locations[1].name = "scatola";
        g_rooms[0].locations[1].look_msg = "Si tratta di una normale scatola di cartone, al suo interno vedi un vecchio **cavo** ed una **tastiera**.";
        {
            g_rooms[0].locations[1].n_objects = 2;
            {
                /* tastiera */
                g_rooms[0].locations[1].objects[0].name = "tastiera";
                g_rooms[0].locations[1].objects[0].locked_look_msg = "Sembra essere una comune tasiera USB.";
                g_rooms[0].locations[1].objects[0].unlocked_look_msg = "Potrei usarla per scrivere qualcosa...";
                g_rooms[0].locations[1].objects[0].take[0] = OBJ_LOCKED_BY_Q;
                g_rooms[0].locations[1].objects[0].take[1] = OBJ_GIVE_TOKEN;
                g_rooms[0].locations[1].objects[0].take[2] = OBJ_UNLOCKED;
                g_rooms[0].locations[1].objects[0].take_q = "Completa la sequenza: 65 83 67 73 ?";
                g_rooms[0].locations[1].objects[0].take_a = "73";
                g_rooms[0].locations[1].objects[0].use_with = &g_rooms[0].locations[0].objects[0]; /* computer */
                g_rooms[0].locations[1].objects[0].use_msg = "Hai connesso la tastiera al computer!";

                /* cavo */
                g_rooms[0].locations[1].objects[1].name = "cavo";
                g_rooms[0].locations[1].objects[1].locked_look_msg = "Sembra essere un doppino telefonico.";
                g_rooms[0].locations[1].objects[1].unlocked_look_msg = "Potrei usarlo per fare qualcosa...";
                g_rooms[0].locations[1].objects[1].take[0] = OBJ_LOCKED_BY_Q;
                g_rooms[0].locations[1].objects[1].take[1] = OBJ_UNLOCKED;
                g_rooms[0].locations[1].objects[1].take_q = "Di che materiale sono i filamenti conduttrici di cui è composto un doppino telefonico?";
                g_rooms[0].locations[1].objects[1].take_a = "rame";
                g_rooms[0].locations[1].objects[1].use_with = &g_rooms[0].locations[0].objects[1]; /* router */
                g_rooms[0].locations[1].objects[1].use_msg = "Hai connesso il router ad internet! Prendilo per riscattare la tua ricompensa.";
            }
        }

        /* libreria */
        g_rooms[0].locations[2].name = "libreria";
        g_rooms[0].locations[2].look_msg = "Tra le decine di libri coglie la tua attenzione un piccolo **calendario**.";
        {
            g_rooms[0].locations[2].n_objects = 1;
            {
                /* calendario */
                g_rooms[0].locations[2].objects[0].name = "calendario";
                g_rooms[0].locations[2].objects[0].unlocked_look_msg = "Sfogliando le pagine del calendario noti che 25/05/2013 è segnata con una X rossa.";
                g_rooms[0].locations[2].objects[0].take[0] = OBJ_UNLOCKED;
                g_rooms[0].locations[2].objects[0].use_with = NULL;
                g_rooms[0].locations[2].objects[0].use_msg = "Non sembra fare nulla.";
            }
        }
    }

    /**
     * SPOILER: Shortest path to win 
     * take cavo (rame)
     * take cavo
     * use cavo router
     * take router
     * take tastiera (73)
     * take tastiera  
     * use tastiera computer
     * drop cavo
     * take password (250513)
     * take password   
     */
    return 0;    
}

struct location* get_location(int room, const char *name) {
    int i;
    for (i = 0; i < g_rooms[room].n_locations; i++) {
        if (strcmp(g_rooms[room].locations[i].name, name) == 0) {
            return &g_rooms[room].locations[i];
        }
    }
    return NULL;
}

struct object* get_object(int room, const char *name) {
    int i;
    for (i = 0; i < g_rooms[room].n_locations; i++) {
        int j;
        for (j = 0; j < g_rooms[room].locations[i].n_objects; j++) {
            if (strcmp(g_rooms[room].locations[i].objects[j].name, name) == 0) {
                return &g_rooms[room].locations[i].objects[j];
            }
        }
    }
    return NULL;
}
