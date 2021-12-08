#ifndef SINGLE_THREAD_PORXY_PROXY_H
#define SINGLE_THREAD_PORXY_PROXY_H

#include <map>
#include <queue>
#include <memory>
#include <sys/socket.h>
#include "Connections/Connection.h"
#include "Constants.h"


class Proxy {
public:
    Proxy() = default;

    explicit Proxy(int listeningPort);

    ~Proxy() = default;

    void run();

    void shutdown();

private:
    int addConnectionFdInPollList(int socketFd, short int events);

    void removeConnectionFdFromPollList(int connectionIdxInPollList);

private:
    int listeningPort;
    bool isInterrupt;

    pollfd pollList[MAX_CONNECTION_NUM] = {0}; //лист опрашиваемых соединений
    int pollListSize;

    int listeningSockPollFdIdx;
    std::priority_queue<int, std::vector<int>, std::ranges::greater> freeIndexesInPollList;
    std::map<int, std::shared_ptr<Connection>> connections; // дескриптор соединения -> в его абстракцию
};


#endif //SINGLE_THREAD_PORXY_PROXY_H
