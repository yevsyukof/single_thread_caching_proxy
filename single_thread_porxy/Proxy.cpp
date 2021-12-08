#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include "Proxy.h"

extern int errno;

void initFreeIndexesInPollListQueue(
        std::priority_queue<int, std::vector<int>, std::ranges::greater> &priorityQueue) {
    for (int i = 0; i < SOMAXCONN; ++i) {
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
    pollList[connectionIdxInPollList].fd = -1;
    pollList[connectionIdxInPollList].events = 0;

    freeIndexesInPollList.push(connectionIdxInPollList);
}

Proxy::Proxy(int listeningPort) : listeningPort(listeningPort),
                                  isInterrupt(false),
                                  pollListSize(0) {
    int reuseFlag = 1;
    sockaddr_in socketAddress{};
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(listeningPort);

    int listeningSocketFd = socket(AF_INET, SOCK_STREAM, 0);
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

    initFreeIndexesInPollListQueue(freeIndexesInPollList);
    // добавляем слушаемый сокет в лист опроса
    listeningSockPollFdIdx = addConnectionFdInPollList(listeningSocketFd, POLLIN);
}

void Proxy::run() {
    int pollStatus;
    while (!isInterrupt) {
        if ((pollStatus = poll(pollList, pollListSize, CONNECTION_CHECK_DELAY)) == -1) {
            std::cerr << "POLL ERROR. Errno = " << errno << std::endl;
            continue;
        }

        if (pollStatus == 0 || pollStatus == -1) {
            // TODo проверяем список соединений
        } else {
            if (pollList[listeningSockPollFdIdx].revents & POLLIN) {
//                acceptNewConnection(); // TODO
            }


        }
    }
}

void Proxy::shutdown() {
    isInterrupt = true;
}

