#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
int main() {
    pid_t pid = getpid();
    printf("I am parent %d\n", pid);
    pid_t pids[5];
    for(int i = 0; i < 5; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            printf("fork failed for child %d\n", i);
        } else if(pids[i] == 0) {
            if(i == 0) {
                printf("I am child %d. I will compile 1.c\n", getpid());
                execlp("gcc", "gcc", "-c", "1.o", "1.c", NULL);
            } else if(i == 1) {
                printf("I am child %d. I will compile 2.c\n", getpid());
                execlp("gcc", "gcc", "-c", "2.o", "2.c", NULL);
            } else if(i == 2) {
                printf("I am child %d. I will compile 3.c\n", getpid());
                execlp("gcc", "gcc", "-c", "3.o", "3.c", NULL);
            } else if(i == 3) {
                printf("I am child %d. I will compile 4.c\n", getpid());
                execlp("gcc", "gcc", "-c", "4.o", "4.c", NULL);
            } else if(i == 4) { 
                printf("I am child %d. I will compile 5.c\n", getpid());
                execlp("gcc", "gcc", "-c", "5.o", "5.c", NULL);
            }
            perror("execlp failed\n");
            exit(-1);
        }
    }

    int status;
    pid_t terminated_pid;
    int success = 1;
    for (int i = 0; i < 5; i++) {
        terminated_pid = waitpid(-1, &status, 0);
        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Child %d terminated with exit status %d\n", 
                    terminated_pid, WEXITSTATUS(status));
                if(WEXITSTATUS(status) != 0) success = 0;
            } else if (WIFSIGNALED(status)) {
                success = 0;
                printf("Child %d terminated by signal %d\n", 
                terminated_pid, WTERMSIG(status));
            }
        }
    }
    if(success) {
        pid_t p = fork();
        if(p == 0) {
            printf("I am child %d. I will link all object files\n", getpid());
            execlp("gcc", "gcc", "-o", "out", "1.o", "2.o", "3.o", "4.o", "5.o", NULL);
        } else {
            waitpid(p, &status, 0);
            printf("Child %d terminated with exit status %d\n", terminated_pid, WEXITSTATUS(status));
            p = fork();
            if(p == 0) {
                printf("I am child %d. I will execute final executable\n", getpid());
                execlp("./out", "./out", NULL);
            } else {
                waitpid(p, &status, 0);
                printf("Child %d terminated with exit status %d\n", terminated_pid, WEXITSTATUS(status));
            }
        }
    } else {
        printf("Failed to compile source files\n");
    }
}