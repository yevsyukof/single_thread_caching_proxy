//
// Created by yevsyukof on 09.12.2021.
//

#ifndef SINGLE_THREAD_PORXY_CONSTANTS_H
#define SINGLE_THREAD_PORXY_CONSTANTS_H

constexpr static int CONNECTION_CHECK_DELAY = 10000;

constexpr static int MAX_CONNECTION_NUM = SOMAXCONN;

constexpr static int IGNORED_SOCKET_FD_VAL = -1; // игнорируется pool при опросе

static const std::string ERROR_MESSAGE_405 = "HTTP/1.0 405 Method Not Allowed\r\nAllow: GET, HEAD\r\n\r\n";

static const std::string ERROR_MESSAGE_500 = "HTTP/1.0 500 Internal Server Error\r\n\r\n";

static const std::string ERROR_MESSAGE_501 = "HTTP/1.0 501 Not Implemented\r\nAllow: GET, HEAD\r\n\r\n";

static const std::string ERROR_MESSAGE_505 = "HTTP/1.0 505 HTTP Version not supported\r\n\r\n";

#endif //SINGLE_THREAD_PORXY_CONSTANTS_H
