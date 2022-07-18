#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 64 MB
#define MAX_RAM_USAGE 67108864

void recv_wrapper(int socket, void *buffer, int message_length) {
    int temp;
    int used_length = 0;
    while (used_length < message_length) {
        temp = recv(socket, buffer + used_length, message_length - used_length, 0);
        used_length += temp;
    }
    return;
}

void create_folder(const char *filename) {
    int max_index = 0;
    char command[1000];
    char folder[1000];
    for (int i = 0; ; i++) {
        switch (filename[i]) {
        case '\0':
            for (int j = 0; j < max_index; j++) {
                folder[j] = filename[j];
            }
            folder[max_index] = 0;
            sprintf(command, "mkdir -p \"%s\"", folder);
            system(command);
            return;
            break;
        case '/':
            max_index = i;
            break;
        }
    }
}


void buffered_writer(const char *filename, int socket, void *buffer, int message_length) {
    create_folder(filename);
    FILE *file = fopen(filename, "wb");
    int next_length;
    int used_length = 0;
    while (used_length < message_length) {
        next_length = (message_length - used_length) > MAX_RAM_USAGE ? MAX_RAM_USAGE : (message_length - used_length);
        recv_wrapper(socket, buffer, next_length);
        used_length += fwrite(buffer, sizeof(char), next_length, file);
    }
    fclose(file);
}

int receive(int socket, const char *path) {
    int file_length;
    int num_files;
    FILE * file;
    char *client_message = NULL;
    char buffer[1000];
    char temp_path[1000];
    char filename[1000];
    char server_message[1000];
    recv(socket, &num_files, sizeof(int), 0);
    for (int i = 0; i < num_files; i++) {
        memset(server_message, 0, sizeof(server_message));
        memset(filename, 0, sizeof(filename));
        memset(temp_path, 0, sizeof(temp_path));
        recv_wrapper(socket, filename, sizeof(filename));
        recv(socket, &file_length, sizeof(int), 0);
        strcpy(temp_path, path);
        strcat(temp_path, filename);
        if (file_length > MAX_RAM_USAGE) {
            client_message = realloc(client_message, MAX_RAM_USAGE * sizeof(char));
            memset(client_message, 0, MAX_RAM_USAGE * sizeof(char));
            printf("streaming '%s' size '%i' bytes\n", filename, file_length);
            buffered_writer(temp_path, socket, client_message, file_length);
        } else {
            client_message = realloc(client_message, file_length * sizeof(char));
            memset(client_message, 0, file_length * sizeof(char));
            recv_wrapper(socket, client_message, file_length * sizeof(char));
            printf("saving '%s' size '%i' bytes\n", filename, file_length);
            create_folder(temp_path);
            file = fopen(temp_path, "wb");
            fwrite(client_message, sizeof(char), file_length, file);
            memset(client_message, 0, file_length * sizeof(char));
            fclose(file);
        }
    }
    return 0;
}


int main(int argc, char *argv[]) {
    const int port = atoi(argv[1]);
    const char *directory = argv[2];
    if (argc < 3) {
        printf("required arguments: port directory\n");
        return -1;
    }
    int socket_description, client_socket, client_address_length;
    struct sockaddr_in server_address, client_address;
    socket_description = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_description == -1) {
        return -1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_description, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        return -1;
    }
    if (listen(socket_description, 1) == -1) {
        return -1;
    }
    client_address_length = sizeof(client_address);
    client_socket = accept(socket_description, (struct sockaddr*)&client_address, (socklen_t*)&client_address_length);
    if (client_socket == -1) {
        return -1;
    }
    receive(client_socket, directory);
    close(client_socket);
    close(socket_description);
    return 0;
}
