#include "trinet.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    if (argc != 3) {
        Log(LOG_ERROR,"Incorrect Arguments", "Please use trinet {hostname} {port}");
        return -1;
    }

    struct Address addr = GetIpAddr(argv[1],atoi(argv[2]));
    if (addr.ip == -1) {
        Log(LOG_ERROR,"Incorrect Arguments", "Bad hostname");
        return -1;
    }
    struct Socket sock = CreateClientSocket(NET_TCP, addr);

    char *msg = "Hello, World!";
    SocketSend(sock, msg, strlen(msg));

    char buf[256];
    SocketRecv(sock,buf,255);
    printf(buf);

    CloseSocket(sock);
}