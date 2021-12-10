#include <iostream>
#include <sys/socket.h>
#include "ServerConnection.h"

#define RECV_BUF_SIZE 16536

#define NOT_FULL_SEND_REQUEST 0
#define FULL_SEND_REQUEST 1
#define SOCKET_SEND_ERROR -1

#define NOT_FULL_RECEIVE_ANSWER NOT_FULL_SEND_REQUEST
#define FULL_RECEIVE_ANSWER FULL_SEND_REQUEST
#define SOCKET_RECEIVE_ERROR SOCKET_SEND_ERROR

ServerConnection::ServerConnection(int connectionSocketFd, int inPollListIdx,
                                   const std::string &requestUrl, const std::string &processedRequestForServer)
        : Connection(connectionSocketFd, inPollListIdx),
          processedRequestForServer(processedRequestForServer),
          sendRequestOffset(0), connectionState(ServerConnectionState::SENDING_REQUEST) {
    this->requestUrl = requestUrl;
}

int ServerConnection::sendRequest() {
    int sendCount;
    if ((sendCount = send(connectionSocketFd,
                          processedRequestForServer.c_str() + sendRequestOffset,
                          processedRequestForServer.length() - sendRequestOffset, 0)) == -1) {
        std::cerr << "SERVER SEND REQUEST ERROR\n";
        return SOCKET_SEND_ERROR;
    } else {
        sendRequestOffset += sendCount;
        if (sendRequestOffset == processedRequestForServer.length()) {
            connectionState = ServerConnectionState::RECEIVING_ANSWER;
            return FULL_SEND_REQUEST;
        } else {
            connectionState = ServerConnectionState::SENDING_REQUEST;
            return NOT_FULL_SEND_REQUEST;
        }
    }
}

bool isResponseStatusIs200(const std::shared_ptr<std::vector<char>>& recvBuf) {
    size_t bodyLen;
    const char *body;
    int http_version_minor, status = 0;
    phr_header headers[100];
    size_t num_headers;

    phr_parse_response(recvBuf->data(), recvBuf->size(),
                       &http_version_minor, &status, &body, &bodyLen, headers, &num_headers, 0);

    return status == 200 && http_version_minor == 0;
}

int ServerConnection::receiveAnswer() {
    char buf[RECV_BUF_SIZE];
    ssize_t recvCount;

    if ((recvCount = recv(connectionSocketFd, buf, RECV_BUF_SIZE, 0)) < 0) {
        std::cerr << "RECEIVE FROM SERVER SOCKET ERROR\n";
        return SOCKET_RECEIVE_ERROR;
    } else {
        recvBuf->insert(recvBuf->end(), buf, buf + recvCount);
        if ((*recvBuf)[recvBuf->size() - 4] == '\r'
            && (*recvBuf)[recvBuf->size() - 3] == '\n'
            && (*recvBuf)[recvBuf->size() - 2] == '\r'
            && (*recvBuf)[recvBuf->size() - 1] == '\n') {
            if (isResponseStatusIs200(recvBuf)) {
                connectionState = ServerConnectionState::CACHING_ANSWER_RECEIVED;
            } else {
                connectionState = ServerConnectionState::NOT_CACHING_ANSWER_RECEIVED;
            }
            return FULL_RECEIVE_ANSWER;
        } else {
            connectionState = ServerConnectionState::RECEIVING_ANSWER;
            return NOT_FULL_RECEIVE_ANSWER;
        }
    }
}
