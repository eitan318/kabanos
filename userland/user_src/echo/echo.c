#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    
    int total_length = 0;
    for (int i = 1; i < argc; i++) {
        total_length += strlen(argv[i]) + 1; // +1 for null terminator
    }

    char *res = (char*)calloc(total_length + 1, sizeof(char)); // +1 for null terminator
    for (int i = 1; i < argc; i++) {
        strcat(res, argv[i]);
        strcat(res, " ");
    }

    printf("%s\n", res);

    free(res);
    return 0;
}
