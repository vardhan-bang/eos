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
                printf("I am child %d. I will terminate successfully\n", getpid());
                execlp("gcc", "gcc", "-o", "main", "main.c", NULL);
            } else if(i == 1) {
                printf("I am child %d. I will terminate unsuccessfully\n", getpid());
                execlp("gcc", "gcc", "abc.c", NULL);
            } else if(i == 2) {
                printf("I am child %d. I will terminate abnormally\n", getpid());
                execlp("pqr", "pqr  ", "main.c", NULL);
            } else if(i == 3) {
                printf("I am child %d. I will terminate successfully\n", getpid());
                execlp("ls", "ls", "-l", NULL);
            } else if(i == 4) { 
                printf("I am child %d. I will terminate from signal\n", getpid());
                while(1);
            }
            perror("execlp failed\n");
            exit(-1);
        }
    }

    int status;
    pid_t terminated_pid;

    for (int i = 0; i < 5; i++) {
        terminated_pid = waitpid(-1, &status, 0);
        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Child %d terminated with exit status %d\n", 
                    terminated_pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child %d terminated by signal %d\n", 
                    terminated_pid, WTERMSIG(status));
            }
        }
    }
}