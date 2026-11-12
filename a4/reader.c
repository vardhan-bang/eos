#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define NAME "/my_shm"
#define PAGE_SIZE 4096
#define SHM_SIZE PAGE_SIZE*3
int main() {
    int fd = shm_open(NAME, O_RDWR, 0600);
    char *p = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    printf("Process %d reading content of each page:\n", getpid());
    for(int i = 0; i < 3; i++) {
        printf("\nPage %d:\n", i);
        printf("%s\n", p+i*PAGE_SIZE);
    }

    sprintf(p+2*PAGE_SIZE+100, "Hello this is response from reader (pid %d)", getpid());
    munmap(p, SHM_SIZE);
}