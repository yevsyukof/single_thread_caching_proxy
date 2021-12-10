#ifndef SINGLE_THREAD_PORXY_HTTPREQUEST_H
#define SINGLE_THREAD_PORXY_HTTPREQUEST_H

#include "HttpParser.h"

struct HttpRequest {
    int version;
    std::string method;
    std::string url;
    phr_header headers[100];
    size_t headersCount;
    std::string host;
};

#endif //SINGLE_THREAD_PORXY_HTTPREQUEST_H
