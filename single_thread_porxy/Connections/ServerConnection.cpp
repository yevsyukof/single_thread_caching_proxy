#include <iostream>
#include <utility>
#include <sys/socket.h>
#include "ServerConnection.h"

#include "../HttpParser/httpresponseparser.h"

#define RECV_BUF_SIZE 16536

#define NOT_FULL_SEND_REQUEST 0
#define FULL_SEND_REQUEST 1
#define SOCKET_SEND_ERROR -1

#define NOT_FULL_RECEIVE_ANSWER NOT_FULL_SEND_REQUEST
#define FULL_RECEIVE_ANSWER FULL_SEND_REQUEST
#define SOCKET_RECEIVE_ERROR SOCKET_SEND_ERROR

ServerConnection::ServerConnection(int connectionSocketFd, int inPollListIdx,
                                   const std::string &requestUrl,
                                   std::shared_ptr<std::string> processedRequestForServer)
        : Connection(connectionSocketFd, inPollListIdx),
          processedRequestForServer(std::move(processedRequestForServer)),
          sendRequestOffset(0), connectionState(ServerConnectionState::SENDING_REQUEST) {
    this->requestUrl = requestUrl;
    // TODO если что move проверить, да и вообще это указатель на память, выделенную ss.str()
}

int ServerConnection::sendRequest() {
    std::cout << "SERVER SEND REQUEST TO SERVER" << std::endl;

    int sendCount;
    if ((sendCount = send(connectionSocketFd,
                          processedRequestForServer->c_str() + sendRequestOffset,
                          processedRequestForServer->size() - sendRequestOffset, 0)) == -1) {
        std::cout << "\n----------------SERVER SEND REQUEST ERROR----------------\n" << std::endl;
        return SOCKET_SEND_ERROR;
    } else {
        sendRequestOffset += sendCount;
        if (sendRequestOffset == processedRequestForServer->length()) {
            connectionState = ServerConnectionState::RECEIVING_ANSWER;

            std::cout << "REQ DOST NA SERVER SYKA" << std::endl;

            return FULL_SEND_REQUEST;
        } else {
            connectionState = ServerConnectionState::SENDING_REQUEST;
            return NOT_FULL_SEND_REQUEST;
        }
    }
}

bool isResponseStatusIs200(const std::shared_ptr<std::vector<char>> &recvBuf) { // TODO
    httpparser::HttpResponseParser httpResponseParser;
    httpparser::Response parsedResponse;

    httpparser::HttpResponseParser::ParseResult parseResult = httpResponseParser
            .parse(parsedResponse, recvBuf->data(), recvBuf->data() + recvBuf->size());

    return parseResult == httpparser::HttpResponseParser::ParsingCompleted && parsedResponse.statusCode == 200;
}

int ServerConnection::receiveAnswer() {
    char buf[RECV_BUF_SIZE];
    ssize_t recvCount;

    if ((recvCount = recv(connectionSocketFd, buf, RECV_BUF_SIZE, 0)) < 0) {
        std::cout << "\n----------------RECEIVE FROM SERVER SOCKET ERROR----------------\n" << std::endl;
        return SOCKET_RECEIVE_ERROR;
    } else if (recvCount > 0) {
        recvBuf->insert(recvBuf->end(), buf, buf + recvCount);
        connectionState = ServerConnectionState::RECEIVING_ANSWER;
        return NOT_FULL_RECEIVE_ANSWER;
    } else {
        if (isResponseStatusIs200(recvBuf)) {
            connectionState = ServerConnectionState::CACHING_ANSWER_RECEIVED;
        } else {
            connectionState = ServerConnectionState::NOT_CACHING_ANSWER_RECEIVED;
        }
        return FULL_RECEIVE_ANSWER;
    }
}
