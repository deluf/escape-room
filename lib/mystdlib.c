
#include "mystdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ssstrlen(const char *buffer, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if (buffer[i] == '\0') {
            return i;
        }
    }
    return -1;
}

char* fgetsnn(char *buffer, int size, FILE *stream) {

    char *ret;
    ret = fgets(buffer, size, stream);

    if (ret != NULL) {
        int i;
        for (i = 0; (i < size - 1) && (buffer[i] != '\n'); i++) ;
        buffer[i] = '\0';
    }

    return ret;
}

void perror_fatal(void) {
    perror(ANSI_COLOR_RED " [Errore]");
    printf(ANSI_COLOR_RESET "\n");
}
