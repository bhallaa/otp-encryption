#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void genKey(char **);

int main(int argc, char *argv[]) {
    if (argc != 1) { genKey(argv); }
    else { printf("ERROR: Not enough arguments provided.\n"); }
}

void genKey(char **arguments) {
    int keyLength = atoi(arguments[1]);
    char *key = malloc((keyLength + 1) * sizeof(char));

    int i, r;
    char c;
    for (i = 0; i < keyLength; i++) {
        r = (rand() % 26) + 64;
        if (r == 64) { c = ' '; }
        else { c = r; }
        key[i] = c;
    }
    key[i] = '\n';
    printf("%s", key);
    free(key);
}
