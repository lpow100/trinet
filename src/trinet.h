#ifndef TRINET_H
#define TRINET_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h> 
#include <unistd.h>

struct Socket {
    void *instance;
    Protocol protocol;
};

typedef enum {
    NET_TCP,
    NET_UDP
} Protocol;

struct Address {
    const char *ip;
    int port;
};

/**
 * @brief Creates a server socket that can accept clients on a certain port
 * * @param backlogLength The length of the backlong for incoming clients, default: 5
 * @return struct Socket A socket for the server or one with a instance of 1 on failure
 */
struct Socket CreateServerSocket(Protocol protocol, int port, int backlogLength);
struct Socket CreateClientSocket(Protocol protocol, struct Address addr);
struct Socket AcceptClient(struct Socket server);
int SocketSend(struct Socket sock, const char *data, int length);
int SocketRecv(struct Socket sock, char* buffer, int maxLength);
void CloseSocket(struct Socket sock);


typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

void Log(LogLevel level, const char *reason, const char *text, ...);

#endif