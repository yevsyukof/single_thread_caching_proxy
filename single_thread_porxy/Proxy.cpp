#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
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
        std::cerr << "BIND LISTENING SOCKET WRONG_REQUEST" << std::endl;
        exit(EXIT_FAILURE);
    }

    // помечаем сокет как слушающий
    if (listen(listeningSocketFd, SOMAXCONN) == -1) {
        std::cerr << "LISTEN FAIL" << std::endl;
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

    std::cout << "NEW CONNECTION: " << acceptedSockFd << std::endl;

    if (acceptedSockFd < 0) {
        std::cerr << "Error accepting connection\n";
        return;
    }

    /// ошибки не обязательно ставить в events, они все равно появлятся в revents
    int inPollListIdx = addConnectionFdInPollList(acceptedSockFd, POLLIN);
    clientsConnections.emplace_back(std::make_shared<ClientConnection>(acceptedSockFd, inPollListIdx));
}

void Proxy::run() {
    while (!isInterrupt) {
        if (poll(pollList, MAX_CONNECTION_NUM, -1) == -1) {
            std::cerr << "Proxy.run(): POLL ERROR. Errno = " << errno << std::endl;
            continue;
        }

        if (pollList[listeningSockInPollListIdx].revents & POLLIN) {
            acceptNewConnection();
        }

        updateClientsConnections(); // TODO
//        updateServersConnections();
    }
}

void Proxy::shutdown() {
    isInterrupt = true;
}

int Proxy::isReadyToSend(int connectionIdxInPollList) {
    return pollList[connectionIdxInPollList].revents & POLLOUT;
}

int Proxy::isReadyToReceive(int connectionIdxInPollList) {
    return pollList[connectionIdxInPollList].revents & POLLIN;
}

bool Proxy::checkConnectionSocketForErrors(int connectionIdxInPollList) {
    if (pollList[connectionIdxInPollList].revents & POLLHUP
        || pollList[connectionIdxInPollList].revents & POLLERR
        || pollList[connectionIdxInPollList].revents & POLLNVAL) {
        return true;
    }
    return false;
}

void Proxy::shutdownClientConnection(const std::shared_ptr<ClientConnection> &clientConnection) {
    switch (clientConnection->getState()) {
        case ClientConnectionStates::WAIT_FOR_RESPONSE : {
            clientsWaitingForResponse[clientConnection->getRequestUrl()].erase(clientConnection);
            break;
        }
        default : {
            break;
        }
    }
    removeConnectionFdFromPollList(clientConnection->getInPollListIdx());
    clientConnection->close();
}

void Proxy::updateClientsConnections() {
    for (auto iterator = clientsConnections.begin(); iterator != clientsConnections.end(); ++iterator) {
        std::shared_ptr<ClientConnection> &clientConnection = *iterator;

        if (checkConnectionSocketForErrors(clientConnection->getInPollListIdx())) {
            shutdownClientConnection(clientConnection);
            iterator = clientsConnections.erase(iterator); // TODO правильный ли будет итератор на след итерации
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
            // TODO создать новое соединение с сервером

        }
    }
}

void Proxy::handleArrivalOfServerResponse(const std::shared_ptr<ClientConnection> &clientConnection) {

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
