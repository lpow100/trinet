#include "trinet.h"

#include <stdlib.h>
#include <string.h>

#define MAX_WAIT_TIME 1000

int server(int argc, char** argv){
    if (argc != 2) {
        Log(LOG_ERROR,"Incorrect Arguments", "Please use trinet server {port}");
    }

    init(0);

    struct Socket sock = CreateServerSocket(NET_TCP, atoi(argv[1]), 5);

    struct Socket client = { 0 };

    char* message = "Type something for the next person to see";
    
    while (true) {
        client = AcceptClient(sock);

        SocketSend(&client, CreatePacket(message,strlen(message),PACK_STR), MAX_WAIT_TIME);

        struct Packet pack = SocketRecv(&client,MAX_WAIT_TIME);
        if (pack.type == PACK_STR) {
            if (pack.data == NULL) {
                Log(LOG_ERROR, "Invalid Packet Data", "Packet data is null.");
                continue;
            }
            if (strcmp(pack.data,"CLOSE_SERVER") == 0) {
                CloseSocket(client);
                break;
            }
            message = malloc(pack.length + 1);
            strcpy(message,pack.data);
            message[pack.length] = '\0';
        } else if (pack.type == PACK_INT) {
            if (pack.data == NULL) {
                Log(LOG_ERROR, "Invalid Packet Data", "Packet data is null.");
                continue;
            }
            printf("%d\n", *(int*)pack.data);
        } else {
            Log(LOG_ERROR, "Invalid Packet", "I don't understand this packet type");
            printf("PACKET: %d\n", pack.type);
            continue;
        }
        free(pack.data);

        CloseSocket(client);
    }
    
    CloseSocket(sock);

    cleanup();

    return 0;
}

int client(int argc, char** argv){
    if (argc != 4) {
        Log(LOG_ERROR,"Incorrect Arguments", "Please use trinet client {hostname} {port} {int | str}");
        return -1;
    }

    init(0);

    struct Address addr = GetIpAddr(argv[1],atoi(argv[2]));
    if (addr.ip == -1) {
        Log(LOG_ERROR,"Incorrect Arguments", "Bad hostname");
        return 1;
    }
    struct Socket sock = CreateClientSocket(NET_TCP, addr);

    struct Packet pack = SocketRecv(&sock,MAX_WAIT_TIME);
    if (pack.data != NULL) {
        printf("%.*s\n", (int)pack.length, (char*)pack.data);
        free(pack.data);
    }

    struct Packet sending = { 0 };
    if (strcmp(argv[3],"int") == 0) {
        int *buff = malloc(255);
        scanf("%d", buff);
        sending.data = buff;
        sending.type = PACK_INT;
    }
    else if (strcmp(argv[3],"str") == 0) {
        char buff[255];
        fgets(buff, 255, stdin); 
        sending.data = buff;
        sending.type = PACK_STR;
    }
    sending.length = 255;
    SocketSend(&sock, sending, MAX_WAIT_TIME);

    CloseSocket(sock);

    cleanup();

    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        Log(LOG_ERROR,"Incorrect Arguments", "Please use trinet {server | client}");
        return 1;
    }
    if (strcmp(argv[1], "server") == 0) {
        return server(argc-1, argv+1);
    } else if (strcmp(argv[1], "client") == 0) {
        return client(argc-1, argv+1);
    } else {
        Log(LOG_ERROR,"Incorrect Arguments", "Please use trinet {server | client}");
        return 1;
    }
}   