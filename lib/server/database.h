
#ifndef LIB_SERVER_DATABASE_H
#define LIB_SERVER_DATABASE_H

enum DB_RESPONSE {
    DB_USERNAME_DOES_NOT_EXIST, /* L'username non esiste */
    DB_READ_FAIL,               /* L'username esiste ma la password è sbagliata */
    DB_READ_SUCCESS,            /* L'username esiste e la password è corretta */
    DB_WRITE_FAIL,              /* Il database non ha abbastanza memoria per inserire il record */
    DB_WRITE_SUCCESS            /* La scrittura del nuovo record è avvenuta con successo */
};

/**
 * Cerca nel database il record (*username*, *password*).
 * Ritorna uno tra:
 *  - DB_USERNAME_DOES_NOT_EXIST 
 *  - DB_READ_FAIL
 *  - DB_READ_SUCCESS
 */ 
enum DB_RESPONSE db_read(const char *username, const char *password);

/**
 * Scrive nel database il record (*username*, *password*).
 * Ritorna uno tra:
 *  - DB_WRITE_FAIL 
 *  - DB_WRITE_SUCCESS
 * Deve essere già stato controllato che *username* non esista nel database.
 */
enum DB_RESPONSE db_write(const char *username, const char *password);

#endif
