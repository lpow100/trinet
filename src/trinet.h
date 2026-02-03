#ifndef TRINET_H
#define TRINET_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h> 
#include <unistd.h>
#include <stdbool.h>

#define QUIET_LOGS 0b1

void init(int flags);
void cleanup();

extern bool quietLogs;

typedef enum {
    NET_TCP,
    NET_UDP
} Protocol;

struct Socket {
    void *instance;
    Protocol protocol;
};

struct Address {
    int ip;
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
struct Address GetHostnameAddr(const char* hostname, int port);
struct Address GetIpAddr(const char* ip, int port);

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

void Log(LogLevel level, const char *reason, const char *text, ...);

#endif