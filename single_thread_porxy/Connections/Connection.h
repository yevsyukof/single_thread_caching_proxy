//
// Created by yevsyukof on 08.12.2021.
//

#ifndef SINGLE_THREAD_PORXY_CONNECTION_H
#define SINGLE_THREAD_PORXY_CONNECTION_H

enum class ConnectionState {
    IN_PROGRESS, OVER, ERROR
};

class Connection {
public:
    explicit Connection(int connectionSocketFd) : socketFd(connectionSocketFd),
                                                  connectionState(ConnectionState::IN_PROGRESS) {};

    bool isAlive() const {
        return connectionState == ConnectionState::IN_PROGRESS;
    };

private:
    int socketFd;
    ConnectionState connectionState;
};

#endif //SINGLE_THREAD_PORXY_CONNECTION_H
