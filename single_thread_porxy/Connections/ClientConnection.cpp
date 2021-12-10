#include <string>
#include <iostream>
#include "ClientConnection.h"

#define RECV_BUF_SIZE 16536

ClientConnection::ClientConnection(int connectionSocketFd, int inPollListIdx)
        : Connection(connectionSocketFd, inPollListIdx),
          connectionState(ClientConnectionStates::WAIT_FOR_REQUEST) {}


void ClientConnection::parseHttpRequestAndUpdateState() {
    const char *path;
    const char *method;
    int http_version_minor; // 1.<minor>
    size_t methodLen, pathLen;

    int reqSize = phr_parse_request(recvRequestBuf.data(), recvRequestBuf.size(), &method, &methodLen,
                                    &path, &pathLen, &http_version_minor, clientHttpRequest.headers,
                                    &clientHttpRequest.headersCount, 0);

    if (reqSize <= -1) {
        std::cerr << "BAD HTTP REQUEST\n";
        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_501;
        return;
    }

    if (http_version_minor != 0) {
        std::cerr << "WRONG HTTP VERSION\n";
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
        std::cerr << "NOT IMPLEMENTED HTTP METHOD\n";
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
        std::cerr << "HASN'T HOST\n";
        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_400;
        return;
    }

    processedRequestForServer += "\r\n\r\n";
    connectionState = ClientConnectionStates::PROCESS_REQUEST;
    requestValidatorState = ClientRequestErrors::WITHOUT_ERRORS;

    requestUrl = clientHttpRequest.method + " " + clientHttpRequest.url;
}

int ClientConnection::receiveRequest() {
    char buf[RECV_BUF_SIZE];
    ssize_t recvCount;

    if ((recvCount = recv(connectionSocketFd, buf, RECV_BUF_SIZE, 0)) < 0) {
        connectionState = ClientConnectionStates::CONNECTION_ERROR;
        std::cerr << "RECEIVE FROM CLIENT SOCKET ERROR\n";
        return SOCKET_RECEIVE_ERROR;
    } else {
        recvRequestBuf.insert(recvRequestBuf.end(), buf, buf + recvCount);
        if (recvRequestBuf[recvRequestBuf.size() - 4] == '\r'
            && recvRequestBuf[recvRequestBuf.size() - 3] == '\n'
            && recvRequestBuf[recvRequestBuf.size() - 2] == '\r'
            && recvRequestBuf[recvRequestBuf.size() - 1] == '\n') {
            parseHttpRequestAndUpdateState();
            return FULL_RECEIVE_REQUEST;
        } else {
            connectionState = ClientConnectionStates::RECEIVE_REQUEST;
            return NOT_FULL_RECEIVE_REQUEST;
        }
    }
}

int ClientConnection::sendAnswer() { // TODO
    int sendCount;
    if ((sendCount = send(connectionSocketFd,
                          sendAnswerBuf->data() + sendAnswerOffset,
                          sendAnswerBuf->size() - sendAnswerOffset, 0)) == -1) {
        std::cerr << "CLIENT SEND ANSWER ERROR\n";
    }

    if (sendCount >= 0) {
        sendAnswerOffset += sendCount;
        if (sendAnswerOffset == sendAnswerBuf->size()) {
            connectionState = ClientConnectionStates::ANSWER_RECEIVED;
            return FULL_SEND_ANSWER;
        } else {
            return NOT_FULL_SEND_ANSWER;
        }
    } else {
        return SOCKET_SEND_ERROR;
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
