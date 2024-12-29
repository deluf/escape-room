
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "../protocol.h"

/**
 * Astrazione di un database, realizzata tramite 
 *  una lista di record (*username*, *password*)
 */
struct record {
    char username[CREDENTIALS_LENGTH_MAX];
    char password[CREDENTIALS_LENGTH_MAX];
    struct record *next;
};

struct record *g_db = NULL;

enum DB_RESPONSE db_read(const char *username, const char *password) {

    struct record *r = g_db;

    while (r != NULL && strcmp(r->username, username) != 0) {
        r = r->next;
    }

    if (r == NULL) {
        return DB_USERNAME_DOES_NOT_EXIST;
    }
    if (strcmp(r->password, password) == 0) {
        return DB_READ_SUCCESS;
    }
    return DB_READ_FAIL;
}

enum DB_RESPONSE db_write(const char *username, const char *password) {

    struct record *r;

    r = malloc(sizeof(struct record));
    if (r == NULL) {
        return DB_WRITE_FAIL;
    }

    strcpy(r->username, username);
    strcpy(r->password, password);

    r->next = g_db;
    g_db = r;
    
    return DB_READ_SUCCESS;
}
