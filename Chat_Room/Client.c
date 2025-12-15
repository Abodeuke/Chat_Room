#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>
#include <ws2tcpip.h>
#include <time.h>
#include <sys/stat.h>

void *receive_messages(void *arg);
void storeChat(char buffer[]);

char name[256];

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    int client_socket;
    struct sockaddr_in server_addr;

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "Connection failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Get the client's name
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strlen(name) - 1] = '\0';  // Remove the newline character

    // Send the client's name to the server
    send(client_socket, name, strlen(name), 0);

    // Create a directory named "logs" if it doesn't exist
    mkdir("logs");

    // Create a thread to receive messages from the server
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_socket) != 0) {
        fprintf(stderr, "Thread creation failed\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Main loop to send messages to the server
    while (1) {
        char message[256];
        fgets(message, sizeof(message), stdin);
        message[strlen(message) - 1] = '\0';  // Remove the newline character
        storeChat(message);

        // Send the message to the server
        send(client_socket, message, strlen(message), 0);

        // Check if the user wants to exit
        if (strcmp(message, "exit") == 0) {
            break;
        }
    }

    // Clean up
    closesocket(client_socket);
    WSACleanup();
    return 0;
}

void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    while (1) {
        char buffer[256];
        ssize_t num_bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if (num_bytes <= 0) {
            printf("\nDisconnected from the server\n");
            break;
        }
        buffer[num_bytes] = '\0';
        storeChat(buffer);
        printf("\n %s \n", buffer);
    }
    closesocket(client_socket);
    pthread_exit(NULL);
}

void storeChat(char buffer[]) {
    time_t current_time;
    struct tm *time_info;
    char time_buffer[80];
    char filename[256];

    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
    sprintf(filename, "logs/%s.txt", name);
    FILE *file = fopen(filename, "a+");
    if (file != NULL) {
        fprintf(file, "%s\t%s\n", buffer, time_buffer);
        fclose(file);
    }
}