#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#define MAX_SEQUENCE_NUMBER 5
#define PORT 9500


void show_error(const char* msg)
{
    perror(msg);
    exit(1);
}

//Nodo 0 genera un pacchetto, che viene mandato in broadcast ai suoi vicini
//Di ogni nodo dobbiamo conoscere l'indirizzo IP e la porta
//Rete peer-to-peer: ogni nodo genera un paccheto ogni 10ms e lo ritrasmette in broadcast
//Se vedo mandare un messaggio con sequence number k, la prossima volta che vedo un messaggio con sequence number k, lo ignoro


//  1-2-0
//   /|\
//  5 4 3

typedef struct Message {
    int sequence_number; 
    int source;
    int *payload; 
    int payload_size;
} Message;

int create_socket(int port)
{
    int sock_fd;
    int broadcast = 1;
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    int ret;

    //Create socket and set options for broadcast
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        show_error("socket");

    if (setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
        show_error("setsockopt");
    
    //Set server address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    //Bind socket
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        show_error("bind");

    return sock_fd;
}

int compare(Message msg1, Message msg2){
    // returns 0 if the messages are the same, else returns 1

    if(msg1.sequence_number == msg2.sequence_number && msg1.source == msg2.source && msg1.payload == msg2.payload && msg1.payload_size == msg2.payload_size){
        return 0;
    }
}

int check_neighbors(int source, int* neighbors, int num_neighbors){

    for(int i = 0; i < num_neighbors; i++){

        if (source == neighbors[i]) return 0;
    }
    return -1;
}

int broadcast_message(int id, int* neighbors, int n_neighbors)
{
    struct sockaddr_in broadcast_addr;
    int addr_len = sizeof(broadcast_addr);
    int sock_fd;
    int ret;
    int count = 0;
    fd_set readfds;
    int i;

    Message msg = {0};
    Message last_msg = {0};

    struct timeval timeout = {0};

    //Set server address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

    //Create socket
    sock_fd = create_socket(PORT);

    //Set timeout
    timeout.tv_sec = 10;

    //Node leader (node with id = 0) generates a message
    if (id == 0)
    {
        sleep(3); //Wait for other nodes to start

        msg.sequence_number = 0;
        msg.source = id;
        msg.payload = (int*)malloc(sizeof(int));
        msg.payload_size = sizeof(int);
        *msg.payload = rand() % 50;

        printf("Node %d: Sending message %d in broadcast\n", id, msg.sequence_number);

        //Send message to neighbors
        if(sendto(sock_fd, &msg, sizeof(msg), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr)) < 0)
            show_error("sendto");
    }

    //Receive and forward messages
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(sock_fd, &readfds);

        ret = select(sock_fd + 1, &readfds, NULL, NULL, &timeout);

        if (ret > 0){
            if (FD_ISSET(sock_fd, &readfds)){

                if(recvfrom(sock_fd, &msg, sizeof(msg), 0, NULL, NULL) < 0)
                    show_error("recvfrom");

                if (msg.source != id && compare(last_msg, msg)){ 
                
                    ret = check_neighbors(msg.source, neighbors, n_neighbors);
                    if (ret == 0){
                        if(msg.sequence_number >= MAX_SEQUENCE_NUMBER) {
                            printf("Max sequence message reached. Closing...\n ");
                            ret = close(sock_fd);
                            if (ret < 0) show_error("close socket error");
                            return 0;
                        }
                        printf("Accepting message %d sent by %d. Sequence: %d\n", msg.payload, msg.source, msg.sequence_number);
                        
                        last_msg = msg;
                        msg.source = id;
                        msg.sequence_number++;

                        printf("Sending %d in broadcast\n", msg.payload);
                        ret = sendto(sock_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
                    }
                }
            }
        }
        else{
            ret = close(sock_fd);
            if (ret < 0) show_error("close socket error");
            return 0;
        }
        
    }
    
}

int main(int argc, char *argv[]){   
    
    int ret;
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 3) {
        show_error("No neighbor for this node");
    }

    int id = atoi(argv[1]);

    int * neighbors = calloc(sizeof(int), argc - 2);
    for (int i = 0; i < argc - 2; i ++) {
        neighbors[i] = atoi(argv[i + 2]);
    }

    printf("Inizializing...\n");
    // Implementation
    int num_neighbors = argc - 2;

    ret = broadcast_message(id, neighbors, num_neighbors);
    if(ret == -1){
            show_error("Error in send_broadcast");
        }
    
    free(neighbors);
    return 0;
}
