#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <netdb.h>
#include <cstring>
#include "Proxy.h"

extern int errno;

void initFreeIndexesInPollListQueue(
        std::priority_queue<int, std::vector<int>, std::ranges::greater> &priorityQueue) {
    for (int i = 0; i < MAX_CONNECTION_NUM; ++i) {
        priorityQueue.push(i);
    }
}

int Proxy::addConnectionFdInPollList(int socketFd, short events) {
    int curFreeIdxInPollList = freeIndexesInPollList.top();
    freeIndexesInPollList.pop();

    ++pollListSize;
    pollList[curFreeIdxInPollList].fd = socketFd;
    pollList[curFreeIdxInPollList].events = events;
    return curFreeIdxInPollList;
}

void Proxy::removeConnectionFdFromPollList(int connectionIdxInPollList) {
    --pollListSize;
    pollList[connectionIdxInPollList].fd = IGNORED_SOCKET_FD_VAL;
    pollList[connectionIdxInPollList].events = 0;
    pollList[connectionIdxInPollList].revents = 0;

    freeIndexesInPollList.push(connectionIdxInPollList);
}

Proxy::Proxy(int listeningPort) : isInterrupt(false),
                                  pollListSize(0) {
    int reuseFlag = 1;
    sockaddr_in socketAddress{};
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(listeningPort);

    listeningSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listeningSocketFd, SOL_SOCKET, SO_REUSEADDR, &reuseFlag, sizeof(reuseFlag));

    // биндим порт только на наш сокет
    if (bind(listeningSocketFd, (struct sockaddr *) &socketAddress, sizeof(socketAddress)) == -1) {
        std::cout << "--------Proxy(): BIND LISTENING SOCKET WRONG_REQUEST--------" << std::endl;
        exit(EXIT_FAILURE);
    }

    // помечаем сокет как слушающий
    if (listen(listeningSocketFd, SOMAXCONN) == -1) {
        std::cout << "--------Proxy(): LISTEN FAIL--------" << std::endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CONNECTION_NUM; ++i) { // заполняем изначальный лист опроса игнорируемыми знач
        pollList[i].fd = IGNORED_SOCKET_FD_VAL;
    }

    initFreeIndexesInPollListQueue(freeIndexesInPollList);
    // добавляем слушаемый сокет в лист опроса
    listeningSockInPollListIdx = addConnectionFdInPollList(listeningSocketFd, POLLIN);
}

void Proxy::acceptNewConnection() {
    int acceptedSockFd = accept(listeningSocketFd, nullptr, nullptr);

    if (acceptedSockFd < 0) {
        std::cout << "--------acceptNewConnection(): Error accepting connection--------" << std::endl;
        return;
    }
    std::cout << "~~~~~~~~~~~~~~~~~~~~ NEW CONNECTION: " << acceptedSockFd << " ~~~~~~~~~~~~~~~~~~~~" << std::endl;

    /// ошибки не обязательно ставить в events, они все равно появлятся в revents
    int inPollListIdx = addConnectionFdInPollList(acceptedSockFd, POLLIN);
    clientsConnections.emplace_back(std::make_shared<ClientConnection>(acceptedSockFd, inPollListIdx));
}

void Proxy::run() {
    while (!isInterrupt) {
        if (poll(pollList, MAX_CONNECTION_NUM, -1) == -1) {
            std::cout << "--------Proxy.run(): POLL ERROR--------" << std::endl;
            continue;
        }

        if (pollList[listeningSockInPollListIdx].revents & POLLIN) {
            acceptNewConnection();
        }

        updateClientsConnections(); // TODO
        updateServersConnections();
    }
}

void Proxy::shutdown() {
    isInterrupt = true;
}

int Proxy::isReadyToSend(int connectionIdxInPollList) const {
    return pollList[connectionIdxInPollList].revents & POLLOUT;
}

int Proxy::isReadyToReceive(int connectionIdxInPollList) const {
    return pollList[connectionIdxInPollList].revents & POLLIN;
}

bool Proxy::checkConnectionSocketForErrors(int connectionIdxInPollList) const {
    if (pollList[connectionIdxInPollList].revents & POLLHUP
        || pollList[connectionIdxInPollList].revents & POLLERR
        || pollList[connectionIdxInPollList].revents & POLLNVAL) {
        return true;
    }
    return false;
}

void Proxy::shutdownClientConnection(const std::shared_ptr<ClientConnection> &clientConnection) {
    switch (clientConnection->getState()) {
        case ClientConnectionStates::WAITING_FOR_RESPONSE : {
            clientsWaitingForResponse[clientConnection->getRequestUrl()].erase(clientConnection);
            break;
        }
        default : {
            break;
        }
    }
    std::cout << "________shutdownClientConnection(): SHUTDOWN CLIENT________\n\n" << std::endl;
    removeConnectionFdFromPollList(clientConnection->getInPollListIdx());
    clientConnection->close();
}

void Proxy::updateClientsConnections() {
    for (auto iterator = clientsConnections.begin(); iterator != clientsConnections.end(); ++iterator) {
        std::shared_ptr<ClientConnection> &clientConnection = *iterator;

        if (checkConnectionSocketForErrors(clientConnection->getInPollListIdx())) {
            shutdownClientConnection(clientConnection);
            iterator = clientsConnections.erase(iterator);
            continue;
        }

        if (isReadyToReceive(clientConnection->getInPollListIdx())) { // можем принять данные из клиентск.
            if (clientConnection->receiveRequest()) {
                if (clientConnection->getState() != ClientConnectionStates::CONNECTION_ERROR) {
                    handleArrivalOfClientRequest(clientConnection);
                } else {
                    shutdownClientConnection(clientConnection);
                    iterator = clientsConnections.erase(iterator);
                    continue;
                }
            }
        } else if (isReadyToSend(clientConnection->getInPollListIdx())) { // будем считать что они взаимоисключаемы
            if (clientConnection->sendAnswer()) {
                shutdownClientConnection(clientConnection);
                iterator = clientsConnections.erase(iterator);
                continue;
            }
        }

        pollList[clientConnection->getInPollListIdx()].revents = 0;
    }
}

void Proxy::handleArrivalOfClientRequest(const std::shared_ptr<ClientConnection> &clientConnection) {
    if (clientConnection->getState() == ClientConnectionStates::WRONG_REQUEST) {
        switch (clientConnection->getError()) {
            case ClientRequestErrors::ERROR_400: {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_400);
                break;
            }
            case ClientRequestErrors::ERROR_405: {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_405);
                break;
            }
            case ClientRequestErrors::ERROR_500: {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_500);
                break;
            }
            case ClientRequestErrors::ERROR_501: {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_501);
                break;
            }
            case ClientRequestErrors::ERROR_504: {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_504);
                break;
            }
            case ClientRequestErrors::ERROR_505: {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_505);
                break;
            }
            case ClientRequestErrors::WITHOUT_ERRORS: {
                throw std::invalid_argument("Недопустимое состояние клиентского соединения");
            }
        }
    } else {
        if (cacheStorage.contains(clientConnection->getRequestUrl())) { /// запись есть
            initializeResponseTransmitting(clientConnection,
                                           cacheStorage.getCacheEntry(clientConnection->getRequestUrl()));
        } else if (clientsWaitingForResponse.find(clientConnection->getRequestUrl()) !=
                   clientsWaitingForResponse.end()) { /// запись качают другие
            clientsWaitingForResponse[clientConnection->getRequestUrl()].insert(clientConnection);
            pollList[clientConnection->getInPollListIdx()].events = 0; // клиента разбудит обработчик прибытия ответа
        } else { /// записи нет
            int newServerConnectionSocketFd = resolveRequiredHost(clientConnection->getClientHttpRequest().host);
            if (newServerConnectionSocketFd >= 0) {
                initializeNewServerConnection(newServerConnectionSocketFd,
                                              clientConnection->getRequestUrl(),
                                              clientConnection->getProcessedRequestForServer());
                addClientInWaitersList(clientConnection);
            } else {
                std::cout << "--------CAN'T RESOLVE HOST--------" << std::endl;
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_504);
            }
        }
    }
}

void Proxy::initializeResponseTransmitting(const std::shared_ptr<ClientConnection> &clientConnection,
                                           const std::string &errorMessage) {
    clientConnection->initializeAnswerSending(errorMessage);
    pollList[clientConnection->getInPollListIdx()].events = POLLOUT;
    clientConnection->setState(ClientConnectionStates::RECEIVING_ANSWER);
}

void Proxy::initializeResponseTransmitting(const std::shared_ptr<ClientConnection> &clientConnection,
                                           const std::shared_ptr<std::vector<char>> &notCachingAnswer) {
    clientConnection->initializeAnswerSending(notCachingAnswer);
    pollList[clientConnection->getInPollListIdx()].events = POLLOUT;
    clientConnection->setState(ClientConnectionStates::RECEIVING_ANSWER);
}

void Proxy::initializeResponseTransmitting(const std::shared_ptr<ClientConnection> &clientConnection,
                                           const CacheEntry &cacheEntry) {
    clientConnection->initializeAnswerSending(cacheEntry);
    pollList[clientConnection->getInPollListIdx()].events = POLLOUT;
    clientConnection->setState(ClientConnectionStates::RECEIVING_ANSWER);
}

void Proxy::shutdownServerConnection(const std::shared_ptr<ServerConnection> &serverConnection) {
    switch (serverConnection->getState()) {
        case ServerConnectionState::SENDING_REQUEST: {
            for (const auto &clientConnection: clientsWaitingForResponse[serverConnection->getRequestUrl()]) {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_504);
                // что то не дало нам получить ответ от вышестоящего сервера
            }
            break;
        }
        case ServerConnectionState::RECEIVING_ANSWER: {
            for (const auto &clientConnection: clientsWaitingForResponse[serverConnection->getRequestUrl()]) {
                initializeResponseTransmitting(clientConnection, ERROR_MESSAGE_504);
                // что то не дало нам получить ответ от вышестоящего сервера
            }
            break;
        }
        case ServerConnectionState::CACHING_ANSWER_RECEIVED: {
            cacheStorage.addCacheEntry(serverConnection->getRequestUrl(),
                                       serverConnection->getRecvBuf());
            for (const auto &clientConnection: clientsWaitingForResponse[serverConnection->getRequestUrl()]) {
                initializeResponseTransmitting(clientConnection,
                                               cacheStorage.getCacheEntry(serverConnection->getRequestUrl()));
            }
            break;
        }
        case ServerConnectionState::NOT_CACHING_ANSWER_RECEIVED: {
            for (const auto &clientConnection: clientsWaitingForResponse[serverConnection->getRequestUrl()]) {
                initializeResponseTransmitting(clientConnection, serverConnection->getRecvBuf());
            }
            break;
        }
    }
    clientsWaitingForResponse.erase(serverConnection->getRequestUrl());

    removeConnectionFdFromPollList(serverConnection->getInPollListIdx());
    serverConnection->close();
}

void Proxy::updateServersConnections() {
    for (auto iterator = serversConnections.begin(); iterator != serversConnections.end(); ++iterator) {
        std::shared_ptr<ServerConnection> &serverConnection = *iterator;

        if (checkConnectionSocketForErrors(serverConnection->getInPollListIdx())) {
            shutdownServerConnection(serverConnection);
            iterator = serversConnections.erase(iterator);
            continue;
        }

        if (isReadyToSend(serverConnection->getInPollListIdx())) { // будем считать что они взаимоисключаемы
            if (serverConnection->sendRequest()) {
                if (serverConnection->getState() == ServerConnectionState::RECEIVING_ANSWER) {
                    // инициализируем принятие ответа от вышестоящего сервера
                    pollList[serverConnection->getInPollListIdx()].events = POLLIN;
                    serverConnection->setState(ServerConnectionState::RECEIVING_ANSWER);
                } else {
                    shutdownServerConnection(serverConnection);
                    iterator = serversConnections.erase(iterator);
                    continue;
                }
            }
        } else if (isReadyToReceive(serverConnection->getInPollListIdx())) { // можем принять данные
            if (serverConnection->receiveAnswer()) {
                shutdownServerConnection(serverConnection);
                iterator = serversConnections.erase(iterator);
                continue;
            }
        }

        pollList[serverConnection->getInPollListIdx()].revents = 0;
    }
}

int Proxy::resolveRequiredHost(const std::string &host) const {
    std::cout << "RESOLVING HOST = " << host << std::endl;

    addrinfo hints{};
    addrinfo *resolvedList = nullptr;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;    /* Разрешены IPv4*/
    hints.ai_socktype = SOCK_STREAM; /* Сокет для дейтаграмм */
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host.c_str(), nullptr, &hints, &resolvedList) != 0) {
        std::cout << "getaddrinfo ERROR" << std::endl;
        return -1;
    }

    int found_socket;
    for (addrinfo *nextFoundAddress = resolvedList; nextFoundAddress != nullptr;
         nextFoundAddress = nextFoundAddress->ai_next) {

        ((sockaddr_in *) nextFoundAddress)->sin_port = htons(80);

        found_socket = socket(nextFoundAddress->ai_family, nextFoundAddress->ai_socktype,
                              nextFoundAddress->ai_protocol);
        if (found_socket == -1) {
            std::cout << "resolveRequiredHost(): Socket error" << std::endl;
            continue;
        }
        if (connect(found_socket, (sockaddr *) &nextFoundAddress, sizeof(*nextFoundAddress)) == 0) {
            std::cout << "resolveRequiredHost(): Can't connect" << std::endl;
            break;
        }
        close(found_socket);
        found_socket = -1;
    }
    freeaddrinfo(resolvedList);

    return found_socket;
}

void Proxy::addClientInWaitersList(const std::shared_ptr<ClientConnection> &clientConnection) {
    clientsWaitingForResponse.at(clientConnection->getRequestUrl()).insert(clientConnection);
    pollList[clientConnection->getInPollListIdx()].events = 0;
}

void Proxy::initializeNewServerConnection(int newServerConnectionSocketFd,
                                          const std::string &requestUrl,
                                          const std::string &processedRequestForServer) {
    int inPollListIdx = addConnectionFdInPollList(newServerConnectionSocketFd, POLLOUT);
    serversConnections.emplace_back(std::make_shared<ServerConnection>(newServerConnectionSocketFd,
                                                                       inPollListIdx,
                                                                       requestUrl,
                                                                       processedRequestForServer));
    clientsWaitingForResponse[requestUrl]; // инициализируем множество ассоц с этим сервером клиентов
}
