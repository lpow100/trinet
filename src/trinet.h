#ifndef TRINET_H
#define TRINET_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h> 
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QUIET_LOGS 0b1

void init(int flags);
void cleanup();

extern bool quietLogs;

typedef enum {
    NET_TCP,
    NET_UDP
} Protocol;

struct Address {
    int ip;
    int port;
};

struct AddressList {
    struct Address *addrs;
    int addrCount;
};

typedef enum {
// Packet status
    PACK_NULL,
    PACK_ERROR,
// Socket status
    PACK_CLOSE,
// Packet handshake
    PACK_ACK,
    PACK_SUCCESS,
// Data
    PACK_INT,
    PACK_STR,
    PACK_FLOAT,
    PACK_RAW
} PacketType;

struct Packet {
    void *data; 
    uint32_t length;          
    PacketType type;   
};

struct SocketGroup {
    struct Socket **members; // Array of pointers to existing sockets
    int count;
    int capacity;
};

#define POLL_WAITING      0x1
#define POLL_PRIORITY     0x2
#define POLL_HUNG_UP      0x4
#define POLL_ERROR        0x8
#define POLL_NOT_OPEN     0x10

struct Socket {
    int instance;
    Protocol protocol;
    int status;
    int lastSent;
    int lastAck;
};

/**
 * @brief Creates a server socket that can accept clients on a certain port
 * * @param protocol the protocol being used for the server
 * * @param port the port being for the servers connections
 * * @param backlogLength The length of the backlong for incoming clients, default: 5
 * @return `struct Socket` A socket for the server or one with a instance of -1 on failure
 */
struct Socket CreateServerSocket(Protocol protocol, int port, int backlogLength);
/**
 * @brief Creates a client socket that connects to a server
 * * @param protocol the protocol being used for the server
 * * @param addr the address of the port
 * @return `struct Socket` A socket for the client that is connected to the server or one with a instance of -1 on failure
 */
struct Socket CreateClientSocket(Protocol protocol, struct Address addr);
/**
 * @brief Waits untill a client joins and then creates a new port for that client
 * * @param server the server that is accepting a client
 * @return `struct Socket` A socket for the client that is connected to or one with a instance of 1 on failure
 */
struct Socket AcceptClient(struct Socket server);

/**
 * @brief Send a packet down a socket
 * * @param sock The socket to send data to
 * * @param pack The packet being sent
 * @return `int` An int where it not being 0 means an error
 */
int SocketSend(struct Socket *sock, struct Packet pack, int maxWaitMs);
/**
 * @brief Recives a packet from a socket
 * * @param sock The socket to send data to
 * @return `struct Packet` The recived packet
 */
struct Packet SocketRecv(struct Socket *sock, int maxWaitMs);
/**
 * @brief Polls the socket and updates the status
 * * @param sock The socket to poll and update
 * @return `int` 0 if the update worked
 */
int PollSocket(struct Socket *sock);
/**
 * @brief Closes a socket
 * * @param sock The socket to close
 */
void CloseSocket(struct Socket sock);

/**
 * @brief Gets the address of a hostname
 * * @param hostname The said hostname, eg: `www.google.com`
 * * @param port The port of the hostname
 * @return `struct Address` The address from the hostname
 */
struct Address GetHostnameAddr(const char* hostname, int port);
/**
 * @brief Gets the address of a ip string
 * * @param ip The string ip eg: `127.0.0.1`
 * * @param port The port of the hostname
 * @return `struct Address` The address from the hostname
 */
struct Address GetIpAddr(const char* ip, int port);

/**
 * @brief Gets the address of a ip string
 * * @param ip The string ip eg: `127.0.0.1`
 * * @param port The port of the hostname
 * @return `struct Address` The address from the hostname
 */
struct Packet CreatePacket(void* data, uint32_t length, PacketType type);

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

void Log(LogLevel level, const char *reason, const char *text, ...);

#endif
