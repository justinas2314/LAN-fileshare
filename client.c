#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 64 MB
#define MAX_RAM_USAGE 67108864

struct vector {
    char **files;
    char **names;
    int length;
};

void copy_into_vector(struct vector *v, const char *file, const char *name) {
    v->names = realloc(v->names, (v->length + 1) * sizeof(char*));
    v->names[v->length] = calloc(strlen(name) + 1, sizeof(char));
    strcpy(v->names[v->length], name);
    v->files = realloc(v->files, (v->length + 1) * sizeof(char*));
    v->files[v->length] = calloc(strlen(file) + 1, sizeof(char));
    strcpy(v->files[v->length++], file);
}

void send_wrapper(int socket, const void *message, int message_length) {
    int temp;
    int used_length = 0;
    while (used_length < message_length) {
        temp = send(socket, message + used_length, message_length - used_length, 0);
        used_length += temp;
    }
    return;
}


void get_files(struct vector *v, const char *path, const char *name) {
    char new_path[1000];
    char new_name[1000];
    struct dirent *read_result;
    DIR *dir = opendir(path);
    while ((read_result = readdir(dir)) != NULL) {
        switch (read_result->d_type) {
            case 4:
                if (strcmp(".", read_result->d_name) && strcmp("..", read_result->d_name)) {
                    sprintf(new_path, "%s/%s", path, read_result->d_name);
                    sprintf(new_name, "%s/%s", name, read_result->d_name);
                    get_files(v, new_path, new_name);
                }
                break;
            case 8:
                sprintf(new_path, "%s/%s", path, read_result->d_name);
                sprintf(new_name, "%s/%s", name, read_result->d_name);
                copy_into_vector(v, new_path, new_name);
                break;
        }
    }
    closedir(dir);
}


void buffered_reader(FILE *file, int socket, void *buffer, int message_length) {
    int next_length;
    int used_length = 0;
    while (used_length < message_length) {
        next_length = (message_length - used_length) > MAX_RAM_USAGE ? MAX_RAM_USAGE : (message_length - used_length);
        used_length += fread(buffer, sizeof(char), next_length, file);
        send_wrapper(socket, buffer, next_length);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("required arguments: ip port directories\n");
        return -1;
    }
    const char *ip = argv[1];
    const int port = atoi(argv[2]); 
    struct vector v;
    v.length = 0;
    v.files = NULL;
    v.names = NULL;
    for (int i = 3; i < argc; i++) {
        get_files(&v, argv[i], "");
    }
    int file_length;
    int socket_description;
    struct sockaddr_in server_address;
    char *client_message = NULL;
    char server_message[1000];
    FILE *file;
    socket_description = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_description == -1) {
        return -1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip);
    if (connect(socket_description, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        return -1;
    }
    send(socket_description, &v.length, sizeof(int), 0);
    for (int i = 0; i < v.length; i++) {
        memset(server_message, 0, 1000 * sizeof(char));
        file = fopen(v.files[i], "rb");
        fseek(file, 0, 2);
        file_length = ftell(file);
        fseek(file, 0, 0);
        send_wrapper(socket_description, v.names[i], 1000 * sizeof(char));
        send(socket_description, &file_length, sizeof(int), 0);
        if (file_length > MAX_RAM_USAGE) {
            client_message = realloc(client_message, MAX_RAM_USAGE * sizeof(char));
            memset(client_message, 0, MAX_RAM_USAGE * sizeof(char));
            printf("streaming '%s' size '%i' bytes\n", v.names[i], file_length);
            buffered_reader(file, socket_description, client_message, file_length);
        } else {
            client_message = realloc(client_message, file_length * sizeof(char));
            memset(client_message, 0, file_length * sizeof(char));
            fread(client_message, sizeof(char), file_length, file);
            printf("sending '%s' size '%i' bytes\n", v.names[i], file_length);
            send_wrapper(socket_description, client_message, file_length * sizeof(char));
        }
        fclose(file);
    }
    return 0;
}
