#ifndef SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
#define SINGLE_THREAD_PORXY_CLIENTCONNECTION_H

#include <vector>
#include "Connection.h"


enum class ClientConnectionState {
    ERROR, // ошибка с соединением
    WAIT_FOR_REQUEST, // соединение еще ничего не прислало
    PROCESS_REQUEST, // соединение в процессе передачи запроса
    WAIT_FOR_ANSWER, // запрос от клиента получен, идет его обработка
    RECEIVING_ANSWER // передача ответа клиенту
};

class ClientConnection : public Connection {
public:
    ClientConnection(int connectionSocketFd, int inPollListIdx);

    int receiveData();

//    ClientConnectionState getState() const {
//        return connectionState;
//    }

private:
    ClientConnectionState connectionState;

};


#endif //SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
