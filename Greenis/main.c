#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_KEY_SIZE 256
#define MAX_VALUE_SIZE 256
#define PORT 7379

#define ok_message "+OK\r\n"
#define error_message "$-1\r\n"

typedef struct Entry {
    char* key;
    char* value;
    struct Entry* next;
    time_t expire_time;
} Entry;

Entry* head = NULL;

void exit_with_error(const char * msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


//Set function
void set(int client_socket, const char* key, const char* value, int expire_time) {
    Entry* entry = malloc(sizeof(Entry));
    entry->key = strdup(key);
    entry->value = strdup(value);
    entry->expire_time = expire_time;
    entry->next = head;
    head = entry;
    write(client_socket, ok_message, strlen(ok_message));
}

//Get function
void get(int client_socket, const char* key) {
    Entry* entry = head;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            char length_str[20];
            sprintf(length_str, "$%lu\r\n", strlen(entry->value));
            write(client_socket, length_str, strlen(length_str));
            write(client_socket, entry->value, strlen(entry->value));
            write(client_socket, "\r\n", 2);
            return;
        }
        entry = entry->next;
    }
    write(client_socket, error_message, strlen(error_message));
    return;
}

//Check expire keys
void* check_expire_keys() {
    while(1) {
        time_t now = time(NULL);
        Entry* entry = head;
        Entry* prev = NULL;
        while (entry) {
            if (entry->expire_time != 0 && entry->expire_time <= now) {
                if (prev == NULL) {
                    head = entry->next;
                } else {
                    prev->next = entry->next;
                }
                free(entry->key);
                free(entry->value);
                free(entry);
                if (prev != NULL) {
                    entry = prev->next;
                } else {
                    entry = head;
                }
            } else {
                prev = entry;
                entry = entry->next;
            }
        }
        sleep(1);
    }
    return NULL;
}

void handle_client(int client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t len = read(client_socket, buffer, sizeof(buffer) - 1);
        if(len <= 0) {
            break;
        }
        char command[MAX_BUFFER_SIZE];
        char key[MAX_KEY_SIZE];
        char value[MAX_VALUE_SIZE];
        int expire_time = 0;

        sscanf(buffer, "*%*d\r\n$%*d\r\n%s\r\n$%*d\r\n%s\r\n$%*d\r\n%s\r\n$%*d\r\n%*s\r\n$%*d\r\n%d\r\n\n", command, key, value, &expire_time);

        if(expire_time != 0) // if expire_time is not set, it will be 0 (0 means no expire time)
            expire_time = time(NULL) + expire_time;

        if(strcmp(command, "SET") == 0) {
            set(client_socket, key, value, expire_time);
        }else if(strcmp(command, "GET") == 0) {
            get(client_socket, key);
        }
        else{
            write(client_socket, ok_message, strlen(ok_message));
        }
    }
    close(client_socket);
    return;
}

int main(int argc, char *argv[]){
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket, 5);
    
    while(1) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
        printf("New client connected\n");

        pid_t pid = fork();
        if(pid == -1) exit_with_error("fork failed");

        if (pid == 0) { 
            close(server_socket); 
            pthread_t expire_thread;
            if(pthread_create(&expire_thread, NULL, check_expire_keys, NULL) != 0) 
                exit_with_error("Failed to create thread");

            handle_client(client_socket);

            if(pthread_cancel(expire_thread) != 0) 
                exit_with_error("Failed to cancel thread");
            if(pthread_join(expire_thread, NULL) != 0) 
                exit_with_error("Failed to join thread");

            printf("Client disconnected\n");
            exit(EXIT_SUCCESS); 
        } else { 
            close(client_socket); 
        }
    }

  

    return 0;
}