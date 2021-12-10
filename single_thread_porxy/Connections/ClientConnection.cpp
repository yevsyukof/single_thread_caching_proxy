#include <string>
#include "ClientConnection.h"

#define RECV_BUF_SIZE 16536

ClientConnection::ClientConnection(int connectionSocketFd, int inPollListIdx)
        : Connection(connectionSocketFd, inPollListIdx),
          connectionState(ClientConnectionStates::WAIT_FOR_REQUEST) {}


void ClientConnection::parseHttpRequest() {
    const char *path;
    const char *method;
    int http_version_minor; // 1.<minor>
    size_t methodLen, pathLen;

    int reqSize = phr_parse_request(recvRequestBuf.data(), recvRequestBuf.size(), &method, &methodLen,
                                    &path, &pathLen, &http_version_minor, clientHttpRequest.headers,
                                    &clientHttpRequest.headersCount, 0);

    if (reqSize <= -1) {
        perror("BAD HTTP REQUEST\n");
        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_501;
        return;
    }

    if (http_version_minor != 0) {
        perror("WRONG HTTP VERSION");
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
        perror("NOT IMPLEMENTED HTTP METHOD");
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
        perror("HASN'T HOST");
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
        perror("RECEIVE FROM CLIENT SOCKET ERROR\n");
        return SOCKET_RECEIVE_ERROR;
    } else {
        recvRequestBuf.insert(recvRequestBuf.end(), buf, buf + recvCount);
        if (recvRequestBuf[recvRequestBuf.size() - 4] == '\r'
            && recvRequestBuf[recvRequestBuf.size() - 3] == '\n'
            && recvRequestBuf[recvRequestBuf.size() - 2] == '\r'
            && recvRequestBuf[recvRequestBuf.size() - 1] == '\n') {
            parseHttpRequest();
            return FULL_RECEIVE_REQUEST;
        } else {
            connectionState = ClientConnectionStates::RECEIVE_REQUEST;
            return NOT_FULL_RECEIVE_REQUEST;
        }
    }
}

int ClientConnection::sendAnswer() {
    return 0; // TODO
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
