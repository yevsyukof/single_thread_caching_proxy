#ifndef SINGLE_THREAD_PORXY_SERVERCONNECTION_H
#define SINGLE_THREAD_PORXY_SERVERCONNECTION_H

#include "Connection.h"

enum class ServerConnectionState {
    SENDING_REQUEST,
    RECEIVING_ANSWER,
    CACHING_ANSWER_RECEIVED,
    NOT_CACHING_ANSWER_RECEIVED
};

class ServerConnection : public Connection {
public:
    ServerConnection(int connectionSocketFd, int inPollListIdx,
                     const std::string &requestUrl,
                     std::shared_ptr<std::string>  processedRequestForServer);

    ServerConnectionState getState() const {
        return connectionState;
    }

    void setState(const ServerConnectionState &state) {
        connectionState = state;
    }

    const std::shared_ptr<std::vector<char>>& getRecvBuf() const {
        return recvBuf;
    }

    int sendRequest();

    int receiveAnswer();

private:
    std::shared_ptr<std::string> processedRequestForServer;
    int sendRequestOffset;

    ServerConnectionState connectionState;
};


#endif //SINGLE_THREAD_PORXY_SERVERCONNECTION_H
