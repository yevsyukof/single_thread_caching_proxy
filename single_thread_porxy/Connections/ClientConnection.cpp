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
          connectionState(ClientConnectionStates::WAITING_FOR_REQUEST),
          processedRequestForServer(nullptr) {}


void ClientConnection::parseRequestAndCheckValidity() {
    httpparser::HttpRequestParser httpRequestParser;
    httpparser::HttpRequestParser::ParseResult parseResult = httpRequestParser
            .parse(clientHttpRequest, recvBuf->data(), recvBuf->data() + recvBuf->size());

    if (parseResult != httpparser::HttpRequestParser::ParsingCompleted) {
        std::cout << "--------CAN'T PARSE HTTP REQUEST--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_501;
        return;
    } else {
        std::cout << "**************** received request: ****************" << std::endl;
        std::cout << clientHttpRequest.inspect() << std::endl;
        std::cout << "**************** end request ****************" << std::endl;
    }

    if (clientHttpRequest.versionMajor != 1 || clientHttpRequest.versionMinor != 0) {
        std::cout << "--------WRONG HTTP VERSION--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_505;
        return;
    }


    if (clientHttpRequest.method != "GET" && clientHttpRequest.method != "HEAD") {
        std::cout << "--------NOT IMPLEMENTED HTTP METHOD--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_405;
        return;
    }

    std::stringstream ss;
    ss << clientHttpRequest.method << " " << clientHttpRequest.uri << " HTTP/"
           << clientHttpRequest.versionMajor << "." << clientHttpRequest.versionMinor << "\r\n";

    bool hasHostHeader = false;
    for (const auto &header: clientHttpRequest.headers) {
        ss << header.name << ": " << header.value << "\r\n";
        if (header.name == "Host") {
            requiredHost = header.value;
            hasHostHeader = true;
        }
    }

    if (!hasHostHeader) {
        std::cout << "--------HASN'T HOST--------" << std::endl;

        connectionState = ClientConnectionStates::WRONG_REQUEST;
        requestValidatorState = ClientRequestErrors::ERROR_400;
        return;
    } else {
        ss << "\r\n";
    }

    connectionState = ClientConnectionStates::PROCESSING_REQUEST;
    requestValidatorState = ClientRequestErrors::WITHOUT_ERRORS;

    processedRequestForServer = std::make_shared<std::string>(ss.str());

    requestUrl = clientHttpRequest.method + " " + clientHttpRequest.uri;
}

int ClientConnection::receiveRequest() {
    char buf[RECV_BUF_SIZE];
    ssize_t recvCount;

    if ((recvCount = recv(connectionSocketFd, buf, RECV_BUF_SIZE, 0)) <= 0) {
        std::cout << "--------RECEIVE FROM CLIENT SOCKET ERROR--------" << std::endl;

        connectionState = ClientConnectionStates::CONNECTION_ERROR;
        return SOCKET_RECEIVE_ERROR;
    } else {
        recvBuf->insert(recvBuf->end(), buf, buf + recvCount);
        if (recvBuf->size() >= 4 && (*recvBuf)[recvBuf->size() - 4] == '\r'
            && (*recvBuf)[recvBuf->size() - 3] == '\n'
            && (*recvBuf)[recvBuf->size() - 2] == '\r'
            && (*recvBuf)[recvBuf->size() - 1] == '\n') {

            parseRequestAndCheckValidity();
            return FULL_RECEIVE_REQUEST;
        } else {
            connectionState = ClientConnectionStates::RECEIVING_REQUEST;
            return NOT_FULL_RECEIVE_REQUEST;
        }
    }
}

int ClientConnection::sendAnswer() {
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
