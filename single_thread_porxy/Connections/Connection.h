#ifndef SINGLE_THREAD_PORXY_CONNECTION_H
#define SINGLE_THREAD_PORXY_CONNECTION_H

#include <vector>
#include <string>
#include <memory>
#include <unistd.h>
#include "sys/socket.h"


class Connection {
public:
    Connection(int connectionSocketFd, int inPollListIdx) : connectionSocketFd(connectionSocketFd),
                                                            inPollListIdx(inPollListIdx) {
        this->recvBuf = std::make_shared<std::vector<char>>(std::vector<char>());
    };

    int close() {
        return ::close(connectionSocketFd);
    }

    int getInPollListIdx() const {
        return inPollListIdx;
    }

    const std::string& getRequestUrl() const {
        return requestUrl;
    }

    bool operator<(Connection &other) const {
        return this->connectionSocketFd < other.connectionSocketFd;
    }

protected:
    int connectionSocketFd;
    int inPollListIdx;

    std::string requestUrl;
    std::shared_ptr<std::vector<char>> recvBuf; // TODO поменля это
};

#endif //SINGLE_THREAD_PORXY_CONNECTION_H
