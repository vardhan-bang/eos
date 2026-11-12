#include <stdio.h>
#include <stdlib.h>

// int global_var = 10;
// const int constant_var = 20;

int main(void) {
    // int local_var = 30;
    long int *ptr = (void*)0x0;

    printf("Accessing invalid region:\n");
    printf("Address = %p\n", (void *)ptr);
    printf("Value before = %ld\n", *ptr);
    *ptr = 50;
    printf("Value after = %ld\n", *ptr);
    while(1) {};
    return 0;
}