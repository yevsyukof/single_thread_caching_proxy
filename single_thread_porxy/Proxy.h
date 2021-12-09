#ifndef SINGLE_THREAD_PORXY_PROXY_H
#define SINGLE_THREAD_PORXY_PROXY_H

#include <map>
#include <queue>
#include <set>
#include <memory>
#include <vector>
#include <sys/socket.h>
#include "Constants.h"
#include "Cache/Cache.h"
#include "Connections/ClientConnection.h"
#include "Connections/ServerConnection.h"


class Proxy {
public:
    Proxy() = default;

    explicit Proxy(int listeningPort);

    ~Proxy() = default;

    void run();

    void shutdown();

private:
    void acceptNewConnection();

    int addConnectionFdInPollList(int socketFd, short int events);

    void removeConnectionFdFromPollList(int connectionIdxInPollList);

    bool isReadyToSend(Connection &connection);

    bool isReadyToReceive(Connection &connection);

    void updateClientsConnections();

    void handleArrivalOfClientRequest(ClientConnection &clientConnection);

    void handleArrivalOfServerResponse(ClientConnection &clientConnection);

private:
    bool isInterrupt;
    int listeningSocketFd;

    pollfd pollList[MAX_CONNECTION_NUM] = {0}; //лист опрашиваемых соединений
    int pollListSize;

    int listeningSockInPollListIdx;
    std::priority_queue<int, std::vector<int>, std::ranges::greater> freeIndexesInPollList;

    std::vector<ClientConnection> clientsConnections;
    std::vector<ServerConnection> serversConnections;

    std::map<ServerConnection, std::set<ClientConnection>> clientsWaitingForResponse;

    Cache cacheStorage;
};


#endif //SINGLE_THREAD_PORXY_PROXY_H
