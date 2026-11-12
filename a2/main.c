#include <stdio.h>
#include <stdlib.h>

int global_var = 10;
const int constant_var = 20;

int main(void) {
    int local_var = 30;
    int *heap_var = malloc(sizeof(int));
    if(heap_var == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }

    *heap_var = 40;
    printf("Address of main function: %p\n", (void *)main);
    printf("Address of global_var: %p\n", (void *)&global_var);
    printf("Address of constant_vat: %p\n", (void *)&constant_var);
    printf("Address of local_var: %p\n", (void *)&local_var);
    printf("Address of heap_var: %p\n", (void *)heap_var);

    printf("\nValues stored:\n");
    printf("global_var = %d\n", global_var);
    printf("constant_var = %d\n", constant_var);
    printf("local_var = %d\n", local_var);
    printf("heap_var = %d\n", *heap_var);
    free(heap_var);
    // printf("Attempting to access invalid virtual memory address:\n");
    // int *p = 0x0;
    // printf("value at 0x0 = %d\n", *p);
    while(1) {};
    return 0;
}