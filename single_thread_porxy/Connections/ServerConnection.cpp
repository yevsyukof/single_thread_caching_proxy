#include "ServerConnection.h"


ServerConnection::ServerConnection(int connectionSocketFd, int inPollListIdx)
        : Connection(connectionSocketFd, inPollListIdx) {}
