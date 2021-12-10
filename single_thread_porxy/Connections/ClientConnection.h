#ifndef SINGLE_THREAD_PORXY_CLIENTCONNECTION_H
#define SINGLE_THREAD_PORXY_CLIENTCONNECTION_H

#include "Connection.h"
#include "../Cache/CacheEntry.h"

enum class ClientConnectionStates {
    CONNECTION_ERROR, // ошибка с соединением
    WRONG_REQUEST, // полученный реквест не валиден
    WAITING_FOR_REQUEST, // соединение еще ничего не прислало
    RECEIVING_REQUEST, // соединение в процессе получения запроса
    PROCESSING_REQUEST, // запрос клиента обрабатывается прокси-сервером
    WAITING_FOR_RESPONSE, // запрос клиента передан на вышестоящий сервер
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

//    void setState(const ClientConnectionStates &state, const ClientRequestErrors &error) {
//        connectionState = state;
//        requestValidatorState = error;
//    }

    void setState(const ClientConnectionStates &state) {
        connectionState = state;
    }

    const HttpRequest& getClientHttpRequest() const {
        return clientHttpRequest;
    }

    const std::string& getProcessedRequestForServer() const {
        return processedRequestForServer;
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
