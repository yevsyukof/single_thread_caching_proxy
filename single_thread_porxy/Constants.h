//
// Created by yevsyukof on 09.12.2021.
//

#ifndef SINGLE_THREAD_PORXY_CONSTANTS_H
#define SINGLE_THREAD_PORXY_CONSTANTS_H

//constexpr static int CONNECTION_CHECK_DELAY = 10000;

constexpr static int MAX_CONNECTION_NUM = 1024; /// ограничение poll(), с большим кол-вом просто не запускается

constexpr static int IGNORED_SOCKET_FD_VAL = -1; // игнорируется pool при опросе

static const std::string ERROR_MESSAGE_400 = "HTTP/1.0 400 Bad Request\r\n\r\n";

static const std::string ERROR_MESSAGE_405 = "HTTP/1.0 405 Method Not Allowed\r\nAllow: GET, HEAD\r\n\r\n";

static const std::string ERROR_MESSAGE_500 = "HTTP/1.0 500 Internal Server Error\r\n\r\n";

static const std::string ERROR_MESSAGE_501 = "HTTP/1.0 501 Not Implemented\r\nAllow: GET, HEAD\r\n\r\n";

static const std::string ERROR_MESSAGE_504 = "HTTP/1.0 504 Gateway Timeout\r\n\r\n";

//static const std::string ERROR_MESSAGE_505 = "HTTP/1.0 505 HTTP Version not supported\r\n\r\n";

static const std::string ERROR_MESSAGE_505 = "HTTP/1.0 505 HTTP Version not supported\r\n\
\r\n<html><head><title>505 HTTP Version not supported</title></head> \
<body><h2>505 HTTP Version not supported</h2><h3>Proxy server does not support the HTTP protocol version used in the request.</h3></body></html>\r\n";


#endif //SINGLE_THREAD_PORXY_CONSTANTS_H
