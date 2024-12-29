
#ifndef LIB_MYSTDLIB_H
#define LIB_MYSTDLIB_H

#include <stdio.h>

/* Colori per l'output da terminale */
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_YELLOW   "\x1b[33m"
#define ANSI_COLOR_RESET    "\x1b[0m"

/** 
 *  fgets no-newline
 * 
 * Legge al massimo *size* - 1 caratteri da *stream* e li 
 *  copia in *buffer*, aggiungendo sempre '\\0' in fondo.
 * 
 * Se non è stato possibile leggere lo *stream* ritorna NULL, altrimenti *str*.
 */
char* fgetsnn(char *buffer, int n, FILE *stream);

/** 
 *  string-safe strlen
 * 
 * Se *buffer* non contiene uno '\\0' nei primi *size* - 1 byte ritorna -1,
 *  altrimenti ritorna il numero di caratteri antecedenti al primo '\\0'.
 */ 
int ssstrlen(const char *buffer, int max_len);

/**
 * Stampa la variabile errno con con colore rosso e prefisso [Error].
 */
void perror_fatal(void);

#endif
