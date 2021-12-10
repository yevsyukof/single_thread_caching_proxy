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
        perror("Error accepting connection");
        return;
    }

    int inPollListIdx = addConnectionFdInPollList(acceptedSockFd, POLLIN);
    clientsConnections.emplace_back(std::make_shared<ClientConnection>(acceptedSockFd, inPollListIdx));
}

void Proxy::run() {
    int pollStatus;
    while (!isInterrupt) {
        if ((pollStatus = poll(pollList, MAX_CONNECTION_NUM, 0)) == -1) {
            std::cerr << "POLL WRONG_REQUEST. Errno = " << errno << std::endl;
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

bool Proxy::isReadyToSend(int connectionIdxInPollList) {
    return (pollList[connectionIdxInPollList].revents & POLLOUT) == POLLOUT;
}

bool Proxy::isReadyToReceive(int connectionIdxInPollList) {
    return (pollList[connectionIdxInPollList].revents & POLLIN) == POLLIN;
}

bool Proxy::checkConnectionSocketForErrors(int connectionIdxInPollList) {
    if (pollList[connectionIdxInPollList].revents & POLLHUP
        || pollList[connectionIdxInPollList].revents & POLLERR
        || pollList[connectionIdxInPollList].revents & POLLNVAL) {
        return true;
    }
    return false;
}

void Proxy::handleClientSocketError(std::shared_ptr<ClientConnection> &clientConnection) {
    switch (clientConnection->getState()) {
        case ClientConnectionStates::WAIT_FOR_REQUEST : {
            break;
        }
        case ClientConnectionStates::PROCESS_REQUEST : {
            break;
        }
        case ClientConnectionStates::WAIT_FOR_ANSWER : {
            clientsWaitingForResponse[clientConnection->getRequestUrl()].erase(clientConnection);
            break;
        }
        case ClientConnectionStates::RECEIVING_ANSWER : {
            clientsWaitingForResponse[clientConnection->getRequestUrl()].erase(clientConnection);
        }
    }
    removeConnectionFdFromPollList(clientConnection->getInPollListIdx());
    clientConnection->close();
}

void Proxy::updateClientsConnections() {
    for (auto iterator = clientsConnections.begin(); iterator != clientsConnections.end(); ++iterator) {
        std::shared_ptr<ClientConnection> &clientConnection = *iterator;

        if (checkConnectionSocketForErrors(clientConnection->getInPollListIdx())) {
            handleClientSocketError(clientConnection);
            clientsConnections.erase(iterator); // TODO правильный ли будет итератор на след итерации
            continue;
        }

        if (isReadyToReceive(clientConnection->getInPollListIdx())) { // можем принять данные из клиентск.
            if (clientConnection->receiveRequest()) {
                if (clientConnection->getState() != ClientConnectionStates::CONNECTION_ERROR) {
                    handleArrivalOfClientRequest(clientConnection);
                } else {
                    handleClientSocketError(clientConnection);
                    clientsConnections.erase(iterator);
                    continue;
                }
            }
        }

        if (isReadyToSend(clientConnection->getInPollListIdx())) {
            // TODO
        }

        pollList[clientConnection->getInPollListIdx()].revents = 0; // нужно ли?
    }
}


void Proxy::handleArrivalOfClientRequest(std::shared_ptr<ClientConnection> &clientConnection) {
    if (clientConnection->getState() == ClientConnectionStates::WRONG_REQUEST) {
        //
    } else {
        // 3 случая кэша?
    }
}

void Proxy::handleArrivalOfServerResponse(std::shared_ptr<ClientConnection> &clientConnection) {

}
