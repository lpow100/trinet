#include "../trinet.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <stdio.h>

bool quietLogs = false;

long long get_ms() {
    return (long long)GetTickCount64();
}

void init(int flags) {
    if ((flags & QUIET_LOGS) != 0) quietLogs = true;

    WSADATA wsadata;
    int initWSA = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (initWSA != 0) {
        printf("WSA failed to initialize %d\n", WSAGetLastError());
        return;
    }
}

void cleanup() { 
    WSACleanup(); 
}

struct Socket CreateServerSocket(Protocol protocol, int port, int backlogLength) {
    struct Socket output = { (int)INVALID_SOCKET, protocol, 0, -1, -1 };
    SOCKET sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket initialization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket binding");
        closesocket(sockfd);
        return output;
    }

    listen(sockfd, backlogLength);
    output.instance = (int)sockfd;
    return output;
}

struct Socket AcceptClient(struct Socket server) {
    struct Socket output = { (int)INVALID_SOCKET, server.protocol, 0, -1, -1 };
    SOCKET clientSockfd;
    int clilen = sizeof(struct sockaddr_in);
    struct sockaddr_in cli_addr;

    clientSockfd = accept((SOCKET)server.instance, (struct sockaddr *)&cli_addr, &clilen);
    if (clientSockfd == INVALID_SOCKET) {
        Log(LOG_ERROR, "Couldn't create client connection socket", "Error in socket acceptance");
        return output;
    }

    output.instance = (int)clientSockfd;
    return output;
}

struct Socket CreateClientSocket(Protocol protocol, struct Address addr) {
    struct Socket output = { (int)INVALID_SOCKET, protocol, 0, -1, -1 };
    SOCKET sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET) {
        Log(LOG_ERROR, "Couldn't create client socket", "Error in socket initialization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.S_un.S_addr = addr.ip;
    serv_addr.sin_port = htons(addr.port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        Log(LOG_ERROR, "Couldn't connect to server", "Error in socket connection");
        closesocket(sockfd);
        return output;
    }

    output.instance = (int)sockfd;
    return output;
}

int recvloop(SOCKET fd, void *buf, size_t n, int flags, int maxMs) {
    size_t total_received = 0;
    uint8_t *ptr = (uint8_t *)buf;
    long long startTime = get_ms();

    while (total_received < n) {
        if ((get_ms() - startTime) >= maxMs) return 0;

        int received = recv(fd, (char *)(ptr + total_received), (int)(n - total_received), flags);
        
        if (received == 0) return 0; 
        if (received == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
            return -1;
        }

        total_received += received;
    }
    return 1;
}

int SocketSend(struct Socket *sock, struct Packet pack, int maxWaitMs) {
    SOCKET portfd = (SOCKET)sock->instance;

    // Build Header
    uint8_t header;
    header = (uint8_t)pack.type;
    uint32_t netLen = htonl(pack.length);
    memcpy(&header, &netLen, 4);

    // Send Header
    if (send(portfd, (char *)header, 5, 0) == SOCKET_ERROR) return -1;

    // Send Body if applicable
    if (pack.length > 0 && pack.data != NULL) {
        if (send(portfd, (char *)pack.data, pack.length, 0) == SOCKET_ERROR) return -1;
    }

    sock->lastSent++;

    // Wait for ACK Packet from receiver
    uint8_t ackBuffer;
    if (recvloop(portfd, ackBuffer, 5, 0, maxWaitMs) <= 0) return 1;

    uint32_t ackLen = ntohl(*(uint32_t*)&ackBuffer);
    PacketType ackType = (PacketType)ackBuffer;

    if (ackType == PACK_CLOSE) {
        sock->status |= POLL_HUNG_UP;
        return 0;
    } else if (ackType == PACK_ACK) {
        sock->lastAck++;
        return 0;
    }
    
    return 1;
}

struct Packet SocketRecv(struct Socket *sock, int maxWaitMs) {
    SOCKET portfd = (SOCKET)sock->instance;
    uint8_t header = {0};
    struct Packet pack = { NULL, 0, PACK_NULL };

    if (recvloop(portfd, header, 5, 0, maxWaitMs) <= 0) return pack;

    uint32_t len = ntohl(*(uint32_t*)&header);
    pack.type = (PacketType)header;
    pack.length = len;

    if (pack.type == PACK_CLOSE) {
        sock->status |= POLL_HUNG_UP;
        return pack;
    }

    if (len > 0) {
        pack.data = malloc(len + 1);
        if (pack.data) {
            recvloop(portfd, pack.data, len, 0, maxWaitMs);
            ((char*)pack.data)[len] = '\0';
        }
    }

    // Send ACK back to sender
    uint8_t ackHeader = { PACK_ACK, 0, 0, 0, 0 }; 
    send(portfd, (char *)ackHeader, 5, 0);

    return pack;
}

int PollSocket(struct Socket *sock) {
    WSAPOLLFD pfd = {0};
    pfd.fd = (SOCKET)sock->instance;
    pfd.events = POLLIN | POLLPRI;

    int ret = WSAPoll(&pfd, 1, 0);
    if (ret == SOCKET_ERROR) return -1;
    if (ret == 0) return 0;

    if (pfd.revents & POLLIN)   sock->status |= POLL_WAITING;
    if (pfd.revents & POLLPRI)  sock->status |= POLL_PRIORITY;
    if (pfd.revents & POLLHUP)  sock->status |= POLL_HUNG_UP;
    if (pfd.revents & POLLERR)  sock->status |= POLL_ERROR;
    if (pfd.revents & POLLNVAL) sock->status |= POLL_NOT_OPEN;

    // Check for graceful closure
    if (pfd.revents & POLLIN) {
        char peek;
        if (recv((SOCKET)sock->instance, &peek, 1, MSG_PEEK) == 0) {
            sock->status |= POLL_HUNG_UP;
        }
    }

    return 1;
}

void CloseSocket(struct Socket sock) { 
    SOCKET portfd = (SOCKET)sock.instance;

    uint8_t header = { (uint8_t)PACK_CLOSE, 0, 0, 0, 0 };
    send(portfd, (char *)header, 5, 0);

    closesocket(portfd); 
}

struct Address GetHostnameAddr(const char* hostname, int port) {
    struct Address output = {0, (uint32_t)port}; 
    struct addrinfo hints = {0}, *res;
    
    hints.ai_family = AF_INET;
    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        Log(LOG_ERROR, "Invalid hostname", "Couldn't find hostname %s", hostname);
        return output;
    }

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    output.ip = ipv4->sin_addr.S_un.S_addr;
    
    freeaddrinfo(res);
    return output;
}

struct Address GetIpAddr(const char* ip, int port) { 
    return (struct Address) { inet_addr(ip), (uint32_t)port }; 
}

struct Packet CreatePacket(void* data, uint32_t length, PacketType type) {
    struct Packet p = { data, length, type };
    return p;
}