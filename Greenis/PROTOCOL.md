Analisi con Wireshark
\*n -> ti sto per mandare un array di n elementi
$m -> stringa lunga m
ok se è tutto ok
-1 se non c'è corrispondenza
server TCP con connessioni contemporanee: processi figli

On Connection

\*4
$6
CLIENT
$7
SETINFO
$8
LIB-NAME
$8
redis-py

+OK

\*4
$6
CLIENT
$7
SETINFO
$7
LIB-VER
$5
5.0.3

+OK

On set(key, value)

\*3
$3
SET
$3
key
$5
value

+OK

On get(key)

\*2
$3
GET
$3
key

$5
value

On get(unexisting key)

\*2
$3
GET
$16
non-existing-key

$-1

On set(key, duration in seconds e.g. 10)

\*5
$3
SET
$3
key
$5
value
$2
EX
$2
10

+OK

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
#define MAX_EXPIRE_KEYS 256
#define PORT 7379

#define ok_message "+OK\r\n"
#define error_message "-1\r\n"

typedef struct {
char key[MAX_KEY_SIZE];
char value[MAX_VALUE_SIZE];
time_t expire_time;
} KeyValuePair;

KeyValuePair kv_store[MAX_EXPIRE_KEYS];
int kv_store_size = 0;

void* check_expire_keys(void* arg) {
while(1) {
time_t now = time(NULL);
for(int i = 0; i < kv_store_size; i++) {
if(kv_store[i].expire_time != 0 && kv_store[i].expire_time <= now) {
kv_store[i].key[0] = '\0';
}
}
sleep(1);
}
return NULL;
}

void handle_client(int client_socket) {
char buffer[MAX_BUFFER_SIZE];
printf("Handle client\n");
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

        // sscanf(buffer, "%s %s %s %d", command, key, value, &expire_time);

        sscanf(buffer, "*%*d\r\n$%*d\r\n%s\r\n$%*d\r\n%s\r\n$%*d\r\n%s\r\n$%*d\r\n%*s\r\n", command, key, value);
        printf("command:%s\nkey:%s\nvalue:%s\n", command,key,value );

        // Convert command to uppercase for comparison
        // for(int i = 0; command[i]; i++){
        //     command[i] = toupper(command[i]);
        // }

        if(strcmp(command, "SET") == 0) {
            int i;
            for(i = 0; i < kv_store_size; i++) {
                if(strcmp(kv_store[i].key, key) == 0) {
                    strncpy(kv_store[i].value, value, sizeof(kv_store[i].value) - 1);
                    kv_store[i].expire_time = expire_time == 0 ? 0 : time(NULL) + expire_time;
                    break;
                }
            }
            if(i == kv_store_size && kv_store_size < MAX_EXPIRE_KEYS) {
                strncpy(kv_store[i].key, key, sizeof(kv_store[i].key) - 1);
                strncpy(kv_store[i].value, value, sizeof(kv_store[i].value) - 1);
                kv_store[i].expire_time = expire_time == 0 ? 0 : time(NULL) + expire_time;
                kv_store_size++;
            }
            write(client_socket, ok_message, strlen(ok_message));
        }else if(strcmp(command, "GET") == 0) {
            printf("GET %s\n", key);
            int key_found = 0;
            for(int i = 0; i < kv_store_size; i++) {
                if(strcmp(kv_store[i].key, key) == 0) {
                    write(client_socket, kv_store[i].value, strlen(kv_store[i].value));
                    key_found = 1;
                    break;
                }
            }
            if (!key_found) {
                write(client_socket, "$-1\r\n", strlen("$-1\r\n"));
            }
        }
        else{
            printf("Invalid command\n");
            write(client_socket, ok_message, strlen(ok_message));
        }
    }
    close(client_socket);

}

int main(int argc, char \*argv[]){
pthread_t expire_thread;
pthread_create(&expire_thread, NULL, check_expire_keys, NULL);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    struct timeval timeout;
    timeout.tv_sec = 10; // 10 second timeout
    timeout.tv_usec = 0;

    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket, 5);



    while(1) {
        printf("Start..\n");
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);

        if (setsockopt (client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
            perror("setsockopt failed\n");

        printf("New client connected\n");

        pid_t pid = fork(); // Create a new process
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { // This block will be executed by the child process
            close(server_socket); // Child does not need the listening socket
            handle_client(client_socket);
            exit(EXIT_SUCCESS); // End the child process
        } else { // This block will be executed by the parent process
            close(client_socket); // Parent does not need the client socket
        }
    }

    return 0;

}
