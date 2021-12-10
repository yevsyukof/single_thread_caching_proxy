#ifndef SINGLE_THREAD_PORXY_PROXY_H
#define SINGLE_THREAD_PORXY_PROXY_H

#include <map>
#include <queue>
#include <set>
#include <list>
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

    int isReadyToSend(int connectionIdxInPollList) const;

    int isReadyToReceive(int connectionIdxInPollList) const;

    bool checkConnectionSocketForErrors(int connectionIdxInPollList) const;

    void updateClientsConnections();

    void shutdownClientConnection(const std::shared_ptr<ClientConnection> &clientConnection);

    void initializeResponseTransmitting(const std::shared_ptr<ClientConnection> &clientConnection,
                                        const std::string &errorMessage);

    void initializeResponseTransmitting(const std::shared_ptr<ClientConnection> &clientConnection,
                                        const std::shared_ptr<std::vector<char>> &notCachingAnswer);

    void initializeResponseTransmitting(const std::shared_ptr<ClientConnection> &clientConnection,
                                        const CacheEntry &cacheEntry);

    void handleArrivalOfClientRequest(const std::shared_ptr<ClientConnection> &clientConnection);

//    void handleArrivalOfServerResponse(const std::shared_ptr<ClientConnection> &clientConnection);

    int resolveRequiredHost(const std::string &host) const;

    void updateServersConnections();

    void shutdownServerConnection(const std::shared_ptr<ServerConnection> &serverConnection);

    void addClientInWaitersList(const std::shared_ptr<ClientConnection> &clientConnection);

    void initializeNewServerConnection(int newServerConnectionSocketFd,
                                       const std::string &requestUrl,
                                       const std::string& processedRequestForServer);


private:
    bool isInterrupt;
    int listeningSocketFd;

    pollfd pollList[MAX_CONNECTION_NUM] = {0}; //лист опрашиваемых соединений
    int pollListSize;

    int listeningSockInPollListIdx;
    std::priority_queue<int, std::vector<int>, std::ranges::greater> freeIndexesInPollList;

    std::list<std::shared_ptr<ClientConnection>> clientsConnections;
    std::list<std::shared_ptr<ServerConnection>> serversConnections;

    std::map<std::string, std::set<std::shared_ptr<ClientConnection>>> clientsWaitingForResponse; // url -> clients

    Cache cacheStorage;
};

#endif //SINGLE_THREAD_PORXY_PROXY_H
