#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>
#include <ws2tcpip.h>
#include <time.h>

#define MAX_CLIENTS 5

struct Client {
    int socket;
    char name[256];
    char ip[INET_ADDRSTRLEN];
};

struct Client clients[MAX_CLIENTS];

void *handle_client(void *arg);

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    pthread_t client_threads[MAX_CLIENTS];
    int num_clients = 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(12345);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 5) == -1) {
        fprintf(stderr, "Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            fprintf(stderr, "Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        char name[256];
        ssize_t name_length = recv(client_socket, name, sizeof(name), 0);
        if (name_length > 0) {
            name[name_length] = '\0';
        } else {
            fprintf(stderr, "Failed to receive client name\n");
            closesocket(client_socket);
            continue;
        }

        if (num_clients < MAX_CLIENTS) {
            clients[num_clients].socket = client_socket;
            strncpy(clients[num_clients].name, name, sizeof(clients[num_clients].name));
            num_clients++;
            printf("Client '%s' connected\n", name);
            pthread_create(&client_threads[num_clients - 1], NULL, handle_client, (void *)&clients[num_clients - 1]);
        } else {
            fprintf(stderr, "Maximum clients reached. Connection rejected.\n");
            closesocket(client_socket);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}

void *handle_client(void *arg) {
    struct Client *client = (struct Client *)arg;
    int client_socket = client->socket;

    while (1) {
        char buffer[256];
        ssize_t num_bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if (num_bytes <= 0) {
            printf("Client '%s' disconnected\n", client->name);
            break;
        }
        buffer[num_bytes] = '\0';
        printf("Received from %s: %s\n", client->name, buffer);
        
        // Broadcast message to all clients (excluding sender)
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients[j].socket != -1 && clients[j].socket != client_socket) {
                send(clients[j].socket, buffer, strlen(buffer), 0);
            }
        }
    }
    closesocket(client_socket);
    pthread_exit(NULL);
}