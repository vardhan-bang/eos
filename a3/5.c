#include <stdio.h>
extern int a1;
extern int a2;
extern int a3;
extern int a4;
int a5 = 50;

int main() {
    printf("1.c: a1 = %d\n", a1);
    printf("2.c: a2 = %d\n", a2);
    printf("3.c: a3 = %d\n", a3);
    printf("4.c: a4 = %d\n", a4);
    printf("5.c: a5 = %d\n", a5);
}