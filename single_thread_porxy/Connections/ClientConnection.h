#ifndef SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
#define SINGLE_THREAD_PORXY_CLIENTCONNECTION_H

#include <vector>
#include "Connection.h"
#include "sys/socket.h"
#include "../HttpParser/HttpRequest.h"


enum class ClientConnectionStates {
//    TERMINATED, // соединение прервано
    CONNECTION_ERROR,
    WRONG_REQUEST, // ошибка с соединением
    WAIT_FOR_REQUEST, // соединение еще ничего не прислало
    PROCESS_REQUEST, // соединение в процессе получения запроса
    WAIT_FOR_ANSWER, // запрос от клиента получен, распарсен и валиден
    RECEIVING_ANSWER // передача ответа клиенту
};

enum class ClientRequestErrors {
    WITHOUT_ERRORS,
    ERROR_405,
    ERROR_500,
    ERROR_501,
    ERROR_505
};

class ClientConnection : public Connection {
public:
    ClientConnection(int connectionSocketFd, int inPollListIdx);

    int receiveRequest();

    ClientConnectionStates getState() const {
        return connectionState;
    }

    void setState(ClientConnectionStates state, ClientRequestErrors error) {
        connectionState = state;
        requestValidatorState = error;
    }

private:
    void parseHttpRequest();

private:
    ClientConnectionStates connectionState;
    ClientRequestErrors requestValidatorState;

    HttpRequest clientHttpRequest;
    std::string processedRequest;
};


#endif //SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
