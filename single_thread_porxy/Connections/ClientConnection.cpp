#include <string>
#include <iostream>
#include "ClientConnection.h"

#define RECV_BUF_SIZE 16536

#define NOT_FULL_RECEIVE_REQUEST 0
#define FULL_RECEIVE_REQUEST 1
#define SOCKET_RECEIVE_ERROR -1

#define NOT_FULL_SEND_ANSWER NOT_FULL_RECEIVE_REQUEST
#define FULL_SEND_ANSWER FULL_RECEIVE_REQUEST
#define SOCKET_SEND_ERROR SOCKET_RECEIVE_ERROR

ClientConnection::ClientConnection(int connectionSocketFd, int inPollListIdx)
        : Connection(connectionSocketFd, inPollListIdx),
          connectionState(ClientConnectionStates::WAITING_FOR_REQUEST) {}


void logClientRequestReceived(const std::vector<char> & a) {
    std::cout << "**************** received request: ****************" << std::endl;
    for (char c : a) {
        std::cout << c;
    }
    std::cout << std::endl;
    std::cout << "**************** end request ****************" << std::endl;
}

void ClientConnection::parseHttpRequestAndUpdateState() {
    const char *path;
    const char *method;
    int http_version_minor; // 1.<minor>
    size_t methodLen, pathLen;

    int reqSize = phr_parse_request(recvBuf->data(), recvBuf->size(), &method, &methodLen,
                                    &path, &pathLen, &http_version_minor, clientHttpRequest.headers,
                                    &clientHttpRequest.headersCount, 0);

    if (reqSize <= -1) {
        std::cout << "--------BAD HTTP REQUEST--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_501;
        return;
    }

    if (http_version_minor != 0) {
        std::cout << "--------WRONG HTTP VERSION--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_505;
        return;
    }

    std::string onlyPath = path;
    onlyPath.erase(onlyPath.begin() + pathLen, onlyPath.end());
    clientHttpRequest.url = onlyPath;

    std::string onlyMethod = method;
    onlyMethod.erase(onlyMethod.begin() + methodLen, onlyMethod.end());
    clientHttpRequest.method = onlyMethod;

    if (clientHttpRequest.method != "GET" && clientHttpRequest.method != "HEAD") {
        std::cout << "--------NOT IMPLEMENTED HTTP METHOD--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_405;
        return;
    }

    processedRequestForServer = onlyMethod + std::string(" ").append(onlyPath).append(" HTTP/1.0") + "\r\n";

    for (int i = 0; i < clientHttpRequest.headersCount; ++i) {
        std::string headerName = clientHttpRequest.headers[i].name;
        headerName.erase(headerName.begin() + clientHttpRequest.headers[i].name_len, headerName.end());
        std::string headerValue = clientHttpRequest.headers[i].value;
        headerValue.erase(headerValue.begin() + clientHttpRequest.headers[i].value_len, headerValue.end());

        if (headerName == "Connection") {
            continue;
        }
        if (headerName == "Host") {
            clientHttpRequest.host = headerValue;
        }

        processedRequestForServer.append(headerName).append(": ").append(headerValue) += "\r\n";
    }

    if (clientHttpRequest.host.empty()) {
        std::cout << "--------HASN'T HOST--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_400;
        return;
    }

    processedRequestForServer += "\r\n\r\n";
    connectionState = ClientConnectionStates::PROCESSING_REQUEST;
    requestValidatorState = ClientRequestErrors::WITHOUT_ERRORS;

    requestUrl = clientHttpRequest.method + " " + clientHttpRequest.url;
}

int ClientConnection::receiveRequest() {
    char buf[RECV_BUF_SIZE];
    ssize_t recvCount;

    if ((recvCount = recv(connectionSocketFd, buf, RECV_BUF_SIZE, 0)) < 0) {
        std::cout << "--------RECEIVE FROM CLIENT SOCKET ERROR--------" << std::endl;

        connectionState = ClientConnectionStates::CONNECTION_ERROR;
        return SOCKET_RECEIVE_ERROR;
    } else {
        recvBuf->insert(recvBuf->end(), buf, buf + recvCount);
        if ((*recvBuf)[recvBuf->size() - 4] == '\r'
            && (*recvBuf)[recvBuf->size() - 3] == '\n'
            && (*recvBuf)[recvBuf->size() - 2] == '\r'
            && (*recvBuf)[recvBuf->size() - 1] == '\n') {

            logClientRequestReceived(*recvBuf);

            parseHttpRequestAndUpdateState(); // TODO
            return FULL_RECEIVE_REQUEST;
        } else {
            connectionState = ClientConnectionStates::RECEIVING_REQUEST;
            return NOT_FULL_RECEIVE_REQUEST;
        }
    }
}

int ClientConnection::sendAnswer() { // TODO
    int sendCount;
    if ((sendCount = send(connectionSocketFd,
                          sendAnswerBuf->data() + sendAnswerOffset,
                          sendAnswerBuf->size() - sendAnswerOffset, 0)) == -1) {
        std::cout << "--------sendAnswer(): SEND ERROR--------" << std::endl;

        connectionState = ClientConnectionStates::CONNECTION_ERROR;
        return SOCKET_SEND_ERROR;
    } else {
        sendAnswerOffset += sendCount;
        if (sendAnswerOffset == sendAnswerBuf->size()) {
            connectionState = ClientConnectionStates::ANSWER_RECEIVED;

            std::cout << "--------sendAnswer(): CLIENT RECEIVE ANSWER--------" << std::endl;

            return FULL_SEND_ANSWER;
        } else {
            std::cout << "--------sendAnswer(): ";
            std::cout << "offset = " << sendAnswerOffset << " fullSize = " << sendAnswerBuf->size() << std::endl;
            std::cout << "--------" << std::endl;

            return NOT_FULL_SEND_ANSWER;
        }
    }
}

void ClientConnection::initializeAnswerSending(const std::string &errorMessage) {
    sendAnswerOffset = 0;
    sendAnswerBuf = std::make_shared<std::vector<char>>(
            std::vector<char>(errorMessage.begin(), errorMessage.end()));
}

void ClientConnection::initializeAnswerSending(const CacheEntry &cacheEntry) {
    sendAnswerOffset = 0;
    sendAnswerBuf = cacheEntry.getCacheEntryData();
}

void ClientConnection::initializeAnswerSending(const std::shared_ptr<std::vector<char>> &notCachingAnswer) {
    sendAnswerOffset = 0;
    sendAnswerBuf = notCachingAnswer;
}
