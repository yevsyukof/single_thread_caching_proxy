#ifndef SINGLE_THREAD_PORXY_CONNECTION_H
#define SINGLE_THREAD_PORXY_CONNECTION_H

#include <vector>
#include <string>
#include <unistd.h>

class Connection {
public:
    Connection(int connectionSocketFd, int inPollListIdx) : connectionSocketFd(connectionSocketFd),
                                                            inPollListIdx(inPollListIdx) {};

    int close() {
        ::close(connectionSocketFd);
    }



    int getInPollListIdx() const {
        return inPollListIdx;
    }

protected:
    int connectionSocketFd;
    int inPollListIdx;

    std::string requestUrl;

    std::vector<char> recvRequestBuf;
};

#endif //SINGLE_THREAD_PORXY_CONNECTION_H
