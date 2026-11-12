#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    long mb = atol(argv[1]);
    size_t bytes = mb * 1024 * 1024;

    char *buffer = (char *)malloc(bytes);
    if (buffer == NULL) {
        fprintf(stderr, "Allocation failed for %ld MB\n", mb);
        exit(1);
    }

    printf("Successfully allocated %ld MB of virtual memory (PID: %d)\n", mb, getpid());

    while(1);
    return 0;
}