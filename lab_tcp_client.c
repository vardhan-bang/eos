#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <Server_IP_Address>\n", argv[0]);
        printf("Example: %s 192.168.1.10\n", argv[0]);
        return 1;
    }

    int sock_fd;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE], reply[BUFFER_SIZE];

    // 1. Create IPv4 TCP Socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("socket"); return 1; }

    // 2. Configure Server IP and Port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address format!\n");
        close(sock_fd);
        return 1;
    }

    // 3. Connect to Instructor's Server PC
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sock_fd);
        return 1;
    }

    printf("Connected to Server %s:%d! Type messages (Ctrl+C to exit):\n", argv[1], PORT);

    // 4. Read from student's keyboard, send to server, print echo reply
    while (fgets(message, sizeof(message), stdin) != NULL) {
        send(sock_fd, message, strlen(message), 0);

        ssize_t n = recv(sock_fd, reply, sizeof(reply) - 1, 0);
        if (n <= 0) {
            printf("Server closed connection.\n");
            break;
        }
        reply[n] = '\0';
        printf("Server Echo: %s", reply);
    }

    close(sock_fd);
    return 0;
}

