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
        std::cerr << "BIND LISTENING SOCKET ERROR" << std::endl;
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
    clientsConnections.emplace_back(acceptedSockFd, inPollListIdx);
}

void Proxy::run() {
    int pollStatus;
    while (!isInterrupt) {
        if ((pollStatus = poll(pollList, MAX_CONNECTION_NUM, 0)) == -1) {
            std::cerr << "POLL ERROR. Errno = " << errno << std::endl;
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

bool Proxy::isReadyToSend(Connection &connection) {
    return pollList[connection.getInPollListIdx()].revents & POLLOUT;
}

bool Proxy::isReadyToReceive(Connection &connection) {
    return pollList[connection.getInPollListIdx()].revents & POLLIN; // >0 == true
}

bool Proxy::checkClientConnectionForErrors(ClientConnection &clientConnection) {
    if (pollList[clientConnection.getInPollListIdx()].revents & POLLHUP) {
        clientConnection.setState(ClientConnectionStates::TERMINATED,
                                  ClientConnectionErrors::WITHOUT_ERRORS);
    } else if (pollList[clientConnection.getInPollListIdx()].revents & POLLERR ||
               pollList[clientConnection.getInPollListIdx()].revents & POLLNVAL) {
        clientConnection.setState(ClientConnectionStates::ERROR,
                                  ClientConnectionErrors::ERROR_500);
    } else {
        return false;
    }
    return true;
}

void Proxy::updateClientsConnections() {
    for (auto iterator = clientsConnections.begin(); iterator != clientsConnections.end(); ++iterator) {
        ClientConnection &clientConnection = *iterator;

        if (pollList[clientConnection.getInPollListIdx()].revents & POLLIN) { // можем принять данные из клиентск.

        }

        pollList[clientConnection.getInPollListIdx()].revents = 0;
    }


//    for (auto &clientConnection : clientsConnections) {
//        if (pollList[clientConnection.getInPollListIdx()].revents & POLLIN) { // можем принять данные из клиентск.
//
//        }
//
//        pollList[clientConnection.getInPollListIdx()].revents = 0;
//    }
}


void Proxy::handleArrivalOfClientRequest(ClientConnection &clientConnection) {

}

void Proxy::handleArrivalOfServerResponse(ClientConnection &clientConnection) {

}
