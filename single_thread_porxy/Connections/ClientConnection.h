#ifndef SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
#define SINGLE_THREAD_PORXY_CLIENTCONNECTION_H

#include "Connection.h"

class ClientConnection : public Connection {
public:
    ClientConnection(int connectionSocketFd);


};


#endif //SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
