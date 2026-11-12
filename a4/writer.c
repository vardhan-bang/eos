#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

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
    getchar();
    printf("\nReading reader's reply:\n");
    printf("%s\n", p+2*page_size+100);
    munmap(p, shm_size);
    shm_unlink(NAME);
}