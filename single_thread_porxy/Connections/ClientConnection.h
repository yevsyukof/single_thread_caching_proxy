#ifndef SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
#define SINGLE_THREAD_PORXY_CLIENTCONNECTION_H

#include <vector>
#include <memory>
#include "Connection.h"
#include "../Cache/CacheEntry.h"
#include "sys/socket.h"
#include "../HttpParser/HttpRequest.h"

#define NOT_FULL_RECEIVE_REQUEST 0
#define FULL_RECEIVE_REQUEST 1
#define SOCKET_RECEIVE_ERROR -1

#define NOT_FULL_SEND_ANSWER NOT_FULL_RECEIVE_REQUEST
#define FULL_SEND_ANSWER FULL_RECEIVE_REQUEST
#define SOCKET_SEND_ERROR SOCKET_RECEIVE_ERROR

enum class ClientConnectionStates {
    CONNECTION_ERROR, // ошибка с соединением
    WRONG_REQUEST, // полученный реквест не валиден
    WAIT_FOR_REQUEST, // соединение еще ничего не прислало
    RECEIVE_REQUEST, // соединение в процессе получения запроса
    PROCESS_REQUEST, // запрос клиента обрабатывается прокси-сервером
    WAIT_FOR_RESPONSE, // запрос клиента передан на вышестоящий сервер
    RECEIVING_ANSWER, // клиент получает ответ
    ANSWER_RECEIVED // ответ передан, соединение отработано
};

enum class ClientRequestErrors {
    WITHOUT_ERRORS,
    ERROR_400,
    ERROR_405,
    ERROR_500,
    ERROR_501,
    ERROR_504,
    ERROR_505
};

class ClientConnection : public Connection {
public:
    ClientConnection(int connectionSocketFd, int inPollListIdx);

    int receiveRequest();

    void initializeAnswerSending(const std::string &errorMessage);

    void initializeAnswerSending(const std::shared_ptr<std::vector<char>> &notCachingAnswer);

    void initializeAnswerSending(const CacheEntry &cacheEntry);

    int sendAnswer();

    ClientConnectionStates getState() const {
        return connectionState;
    }

    ClientRequestErrors getError() const {
        return requestValidatorState;
    }

    void setState(const ClientConnectionStates &state, const ClientRequestErrors &error) {
        connectionState = state;
        requestValidatorState = error;
    }

    void setState(const ClientConnectionStates &state) {
        connectionState = state;
    }

private:
    void parseHttpRequestAndUpdateState();

private:
    ClientConnectionStates connectionState;
    ClientRequestErrors requestValidatorState;

    HttpRequest clientHttpRequest;
    std::string processedRequestForServer;

    std::shared_ptr<std::vector<char>> sendAnswerBuf;
    int sendAnswerOffset;
};


#endif //SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
