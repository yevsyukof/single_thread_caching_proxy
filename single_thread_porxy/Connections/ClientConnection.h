#ifndef SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
#define SINGLE_THREAD_PORXY_CLIENTCONNECTION_H

#include <vector>
#include "Connection.h"
#include "sys/socket.h"


enum class ClientConnectionStates {
    TERMINATED, // соединение прервано
    ERROR, // ошибка с соединением
    WAIT_FOR_REQUEST, // соединение еще ничего не прислало
    PROCESS_REQUEST, // соединение в процессе получения запроса
    WAIT_FOR_ANSWER, // запрос от клиента получен, идет его обработка
    RECEIVING_ANSWER // передача ответа клиенту
};

enum class ClientConnectionErrors {
    WITHOUT_ERRORS,
    ERROR_405,
    ERROR_500,
    ERROR_501,
    ERROR_505
};

class ClientConnection : public Connection {
public:
    ClientConnection(int connectionSocketFd, int inPollListIdx);

    ssize_t receiveData();

    ClientConnectionStates getState() const {
        return connectionState;
    }

    void setState(ClientConnectionStates state, ClientConnectionErrors error) {
        connectionState = state;
        errorState = error;
    }

private:
    ClientConnectionStates connectionState;
    ClientConnectionErrors errorState;
};


#endif //SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
