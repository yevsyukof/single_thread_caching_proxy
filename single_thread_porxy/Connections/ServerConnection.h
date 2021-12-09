#ifndef SINGLE_THREAD_PORXY_SERVERCONNECTION_H
#define SINGLE_THREAD_PORXY_SERVERCONNECTION_H

#include "Connection.h"

enum class ServerConnectionState {
    REQUEST_DONE,
    PROCESS_REQUEST,
    ERROR
};

class ServerConnection : public Connection {
public:
    ServerConnection(int connectionSocketFd, int inPollListIdx);
};


#endif //SINGLE_THREAD_PORXY_SERVERCONNECTION_H
