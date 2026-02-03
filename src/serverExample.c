#include "trinet.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    printf("Hi!\n`");
    if (argc != 2) {
        Log(LOG_ERROR,"Incorrect Arguments", "Please use trinet {port}");
    }

    init(0);

    struct Socket sock = CreateServerSocket(NET_TCP, atoi(argv[1]), 5);

    struct Socket client = AcceptClient(sock);

    char buf[256];
    SocketRecv(client,buf,255);
    printf(buf);

    char *msg = "Hello, Worlder!";
    SocketSend(client, msg, strlen(msg));

    CloseSocket(sock);
    CloseSocket(client);

    cleanup();
}