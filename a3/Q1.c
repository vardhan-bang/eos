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
                pid_t pid = getpid();
                printf("I am child %d\n", pid);
                int temp;
                for(int j = 0; j < 10000000; j++) {
                    temp = temp + i;
                }
                exit(0);
            } else if(i == 1) {
                pid_t pid = getpid();
                printf("I am child %d\n", pid);
                int temp;
                for(int j = 0; j < 1000000; j++) {
                    temp = temp + i;
                }
                exit(1);
            } else if(i == 2) {
                pid_t pid = getpid();
                printf("I am child %d\n", pid);
                while(1);
            } else if(i == 3) {
                pid_t pid = getpid();
                printf("I am child %d\n", pid);
                while(1);
            } else if(i == 4) {
                pid_t pid = getpid();
                printf("I am child %d\n", pid);
                exit(42);
            }
        }
    }

    int status;
    pid_t terminated_pid;

    for (int i = 0; i < 5; i++) {
        terminated_pid = waitpid(-1, &status, 0);
        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Child %d terminated normally with exit status %d\n", 
                    terminated_pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child %d terminated abnormally by signal %d\n", 
                    terminated_pid, WTERMSIG(status));
            }
        }
    }
}