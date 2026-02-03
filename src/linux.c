#include <trinet.h>

void Log(LogLevel level, const char *reason, const char *text, ...) {
    va_list args;
    va_start(args, text);

    switch (level) {
        case LOG_INFO:    printf("[INFO]: "); break;
        case LOG_WARNING: printf("[WARNING]: "); break;
        case LOG_ERROR:   printf("[ERROR]: "); break;
        case LOG_DEBUG:   printf("[DEBUG]: "); break;
    }

    printf("%s: ", reason);
    vprintf(text, args);
    printf("\n");

    va_end(args);
}

struct Socket CreateServerSocket(Protocol protocol, int port, int backlogLength) {
    struct Socket output = { (intptr_t)-1, protocol};
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

    output.instance = (intptr_t)sockfd;
    return output;
}

struct Socket AcceptClient(struct Socket server) {
    struct Socket output = { (intptr_t)-1, server.protocol};
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

    output.instance = (intptr_t)clientSockfd;
    return output;
}

struct Socket CreateClientSocket(Protocol protocol, struct Address addr) {
    struct Socket output = { (intptr_t)-1, protocol};
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET /*IPv4 domain*/, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Log(LOG_ERROR, "Couldn't create server socket", "Error in socket initalization");
        return output;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addr.ip);
    serv_addr.sin_port = htons(addr.port);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        Log(LOG_ERROR, "Couldn't connect to server", "Error in socket connection");
        return output;
    }

    output.instance = (intptr_t)sockfd;
    return output;
}

int SocketSend(struct Socket sock, const char *data, int length) { return write((int)sock.instance,data,length); }
int SocketRecv(struct Socket sock, char* buffer, int maxLength) { return recv((int)sock.instance,buffer,maxLength); }
void CloseSocket(struct Socket sock) { close((int)sock.instance); }