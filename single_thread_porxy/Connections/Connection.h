#ifndef SINGLE_THREAD_PORXY_CONNECTION_H
#define SINGLE_THREAD_PORXY_CONNECTION_H

#include <vector>
#include <string>

class Connection {
public:
    Connection(int connectionSocketFd, int inPollListIdx) : connectionSocketFd(connectionSocketFd),
                                                            inPollListIdx(inPollListIdx) {};

    int getInPollListIdx() const {
        return inPollListIdx;
    }

protected:
    int connectionSocketFd;
    int inPollListIdx;

    std::string requestUrl;

    std::vector<char> receiveDataBuf;
};

#endif //SINGLE_THREAD_PORXY_CONNECTION_H
