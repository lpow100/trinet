#include "trinet.h"
#include <winsock2.h>

void init(int flags) {
    if ((flags & QUIET_LOGS) != 0) quietLogs = false;

    WSADATA wsadata;

    int initWSA = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (initWSA != 0) {
        printf("WSA failed to initialize %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }
}
void cleanup() { WSACleanup(); }

struct Socket CreateServerSocket(Protocol protocol, int port, int backlogLength) {
    struct Socket output = { (void*)(intptr_t)-1, protocol};
    SOCKET sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET /*IPv4 domain*/, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket initalization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket binding");
        return output;
    }

    listen(sockfd,backlogLength);

    output.instance = (void*)(intptr_t)sockfd;
    return output;
}

struct Socket AcceptClient(struct Socket server) {
    struct Socket output = { (void*)(intptr_t)-1, server.protocol};
    SOCKET clientSockfd;
    int clilen;
    struct sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);
    clientSockfd = accept((SOCKET)server.instance, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
    if (clientSockfd == INVALID_SOCKET) {
        Log(LOG_ERROR, "Couldn't create client connection socket", "Error in socket acception");
        return output;
    }

    output.instance = (void*)(intptr_t)clientSockfd;
    return output;
}

struct Socket CreateClientSocket(Protocol protocol, struct Address addr) {
    struct Socket output = { (void*)(intptr_t)-1, protocol};
    SOCKET sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET /*IPv4 domain*/, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket initalization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.S_un.S_addr = addr.ip;
    serv_addr.sin_port = htons(addr.port);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        Log(LOG_ERROR, "Couldn't connect to server", "Error in socket connection");
        return output;
    }

    output.instance = (void*)(intptr_t)sockfd;
    return output;
}

int SocketSend(struct Socket sock, const char *data, int length) { return send((SOCKET)sock.instance,data,length, 0); }
int SocketRecv(struct Socket sock, char* buffer, int maxLength) { return recv((SOCKET)sock.instance,buffer,maxLength,0); }
void CloseSocket(struct Socket sock) { closesocket((SOCKET)sock.instance); }

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