#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

#define NAME "/my_shm"

int main() {
    int page_size = sysconf(_SC_PAGE_SIZE);
    int shm_size = page_size*3;
    int fd = shm_open(NAME, O_CREAT | O_RDWR, 0600);
    ftruncate(fd, shm_size);
    char *p = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    for(int i = 0; i < 3; i++) {
        char *page_pointer = p + i*page_size;
        sprintf(page_pointer, "Page %d (written by %d)\n", i, getpid());
    }

    pid_t child = fork();
    if(child == 0) {
        printf("Child %d reading pages:\n", getpid());
        for(int i = 0; i < 3; i++) {
            char *page = p + i*page_size;
            printf("\nPage %d:\n", i);
            printf("%s\n", page);
        }
        munmap(p, shm_size);
        exit(0);
    }
    int status;
    waitpid(child, &status, 0);
    munmap(p, shm_size);
    getchar();
    shm_unlink(NAME);

}