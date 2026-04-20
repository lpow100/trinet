#define _POSIX_C_SOURCE 199309L
#include "../trinet.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <time.h>

bool quietLogs = false;

long long get_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void init(int flags) {
    if ((flags & QUIET_LOGS) != 0) {
        quietLogs = true;
    }
}
void cleanup() { /*Fuck windows api*/ }

struct Socket CreateServerSocket(Protocol protocol, int port, int backlogLength) {
    struct Socket output = { -1, protocol};
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET /*IPv4 domain*/, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket initalization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket binding");
        return output;
    }

    listen(sockfd,backlogLength);

    output.instance = sockfd;
    return output;
}

struct Socket AcceptClient(struct Socket server) {
    struct Socket output = { -1, server.protocol};
    int clientSockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);
    clientSockfd = accept((int)server.instance, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
    if (clientSockfd < 0) {
        Log(LOG_ERROR, "Couldn't create client connection socket", "Error in socket acception");
        return output;
    }

    output.instance = clientSockfd;
    return output;
}

struct Socket CreateClientSocket(Protocol protocol, struct Address addr) {
    struct Socket output = { -1, protocol, 0, -1, -1};
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET /*IPv4 domain*/, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket initalization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = addr.ip;
    serv_addr.sin_port = htons(addr.port);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        Log(LOG_ERROR, "Couldn't connect to server", "Error in socket connection");
        return output;
    }

    output.instance = sockfd;
    return output;
}

int recvloop(int fd, void *buf, size_t n, int flags, int maxMs) {
    size_t total_received = 0;
    uint8_t *ptr = (uint8_t *)buf; // Cast to byte-pointer for math
    int startTime = get_ms();
    int currentTime = get_ms();

    while (total_received < n && (currentTime - startTime) < (maxMs)) {
        ssize_t received = recv(fd, ptr + total_received, n - total_received, flags);
        
        if (received == 0) return 0;  // Connection closed gracefully
        if (received < 0) return -1; // Socket error (e.g., timeout or reset)

        total_received += received;
        currentTime = get_ms();
    }
    if ((startTime - currentTime) < maxMs) return 1;
    return 0;
}

int SocketSend(struct Socket *sock, struct Packet pack, int maxWaitMs) {
    int portfd = (int)sock->instance;

    uint8_t header[5];
    header[0] = (uint8_t)pack.type;
    uint32_t netLen = htonl(pack.length);
    memcpy(&header[1], &netLen, 4);

    uint8_t ackheader[5];
    ackheader[0] = (uint8_t)PACK_ACK;
    uint32_t acknetLen = htonl(0);
    memcpy(&ackheader[1], &acknetLen, 4);

    struct iovec iov[3];
    int iovcnt = 0;

    // The header is ALWAYS sent
    iov[0].iov_base = header;
    iov[0].iov_len  = 5;
    iovcnt = 1;

    switch (pack.type) {
        case PACK_FLOAT: case PACK_INT: 
        case PACK_STR:   case PACK_RAW:
            if (pack.length > 0 && pack.data != NULL) {
                iov[1].iov_base = pack.data;
                iov[1].iov_len  = pack.length;
                iovcnt = 2;
            }
            break;
        case PACK_ACK:
        case PACK_CLOSE: 
            break;
        default:
            return 1;
    }

    struct msghdr msg = {0};
    msg.msg_iov = iov;
    msg.msg_iovlen = iovcnt;

    ssize_t sent = sendmsg(portfd, &msg, MSG_NOSIGNAL);
    sock->lastSent++;

    if (sent < 0) return -1;

    uint8_t buffer[5];
    recvloop(portfd,buffer,5,0, maxWaitMs);

    uint32_t len = ((uint32_t)buffer[1] << 24) | 
                ((uint32_t)buffer[2] << 16) | 
                ((uint32_t)buffer[3] << 8)  | 
                    (uint32_t)buffer[4];
    struct Packet packRecv = { 0 };
    packRecv.data = (void*)NULL;
    packRecv.length = len;
    packRecv.type = (PacketType)buffer[0];

    if (buffer[0] == PACK_CLOSE) {
        sock->status |= POLL_HUNG_UP;
        return 0;
    } else if (buffer[0] == PACK_ACK) {
        sock->lastAck++;
        return 0;
    }
    
    return 1;
}

/*struct Packet SocketRecv(struct Socket sock, int maxWaitMs) {
    int portfd = (int)sock.instance;

    uint8_t buffer[5];
    recvloop(portfd,buffer,5,0, maxWaitMs);

    uint32_t len = ((uint32_t)buffer[1] << 24) | 
                   ((uint32_t)buffer[2] << 16) | 
                   ((uint32_t)buffer[3] << 8)  | 
                    (uint32_t)buffer[4];
    struct Packet pack = {(void*)buffer[0], len, PACK_NULL};

    if (buffer[0] == PACK_CLOSE) {
        sock.status |= POLL_HUNG_UP;
        return pack;
    } else if (buffer[0] == PACK_ACK) {
        uint8_t header[5] = { PACK_SUCCESS, 0, 0, 0, 0 }; 
        send(portfd, header, 5, 0);
        sock.lastAck++;
        return pack;
    } else if (buffer[0] == PACK_SUCCESS) {
        return pack;
    }

    if (len > 0) {
        pack.data = malloc(len + 1); // +1 for a manual null terminator
        if (pack.data) {
            recvloop(portfd, pack.data, len, 0, maxWaitMs);
            ((char*)pack.data)[len] = '\0'; // Manually null terminate for safety
        }
    } else {
        pack.data = NULL;
    }


    uint8_t header[5] = { PACK_ACK, 0, 0, 0, 0 }; 
    send(portfd, header, 5, 0);

    return pack;
}*/

struct Packet SocketRecv(struct Socket *sock, int maxWaitMs) {
    int portfd = (int)sock->instance;
    uint8_t buffer[5];
    recvloop(portfd,buffer,5,0, maxWaitMs);

    uint32_t len = ((uint32_t)buffer[1] << 24) | 
                   ((uint32_t)buffer[2] << 16) | 
                   ((uint32_t)buffer[3] << 8)  | 
                    (uint32_t)buffer[4];
    struct Packet pack = {(void*)buffer[0], len, buffer[0]};

    if (pack.type == PACK_CLOSE) {
        sock->status |= POLL_HUNG_UP;
        pack.type = PACK_CLOSE;
        return pack;
    }

    if (len > 0) {
        pack.data = malloc(len + 1);
        if (pack.data) {
            recvloop(portfd, pack.data, len, 0, maxWaitMs);
            ((char*)pack.data)[len] = '\0';
        }
    }

    uint8_t header[5] = { PACK_ACK, 0, 0, 0, 0 }; 
    send(portfd, header, 5, 0);

    return pack;
}

int PollSocket(struct Socket *sock) {
    struct pollfd pfd = {
        (int)sock->instance,
        POLLIN | POLLPRI,
        0
    };

    int ret = poll(&pfd, 1, 0);
    if (ret < 0) return -1;
    if (ret == 0) return 0;

    if (pfd.revents & POLLIN)   sock->status |= POLL_WAITING;
    if (pfd.revents & POLLPRI)  sock->status |= POLL_PRIORITY;
    if (pfd.revents & POLLHUP)  sock->status |= POLL_HUNG_UP;
    if (pfd.revents & POLLERR)  sock->status |= POLL_ERROR;
    if (pfd.revents & POLLNVAL) sock->status |= POLL_NOT_OPEN;

    if (pfd.revents & POLLIN) {
        char peek;
        if (recv((int)sock->instance, &peek, 1, MSG_PEEK) == 0) {
            sock->status |= POLL_HUNG_UP;
        }
    }

    return 1;
}

void CloseSocket(struct Socket sock) { 
    int portfd = (int)sock.instance;

    uint8_t header[5];
    header[0] = (uint8_t)PACK_CLOSE;
    uint32_t netLen = htonl(0);
    memcpy(&header[1], &netLen, 4);

    struct iovec iov[1];
    iov[0].iov_base = header;
    iov[0].iov_len  = 5;

    struct msghdr msg = {0};
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    sendmsg(portfd, &msg, MSG_NOSIGNAL);

    close(portfd); 
}

struct Address GetHostnameAddr(const char* hostname, int port) {
    struct Address output = {-1, -1}; 

    struct hostent *addr;
    addr = gethostbyname(hostname);
    if (addr == NULL) {
        Log(LOG_ERROR, "Invalid hostname", "Couldn't find hostname %s", hostname);
        return output;
    }

    output.ip = inet_addr(addr->h_addr_list[0]);
    output.port = port;
    return output;
}

struct Address GetIpAddr(const char* ip, int port) { return (struct Address) {inet_addr(ip),port}; }

struct Packet CreatePacket(void* data, uint32_t length, PacketType type) {
    struct Packet p;
    p.data = data;
    p.length = length;
    p.type = type;
    return p;
}