#include <iostream>
#include <csignal>
#include <memory.h>
#include <sys/poll.h>
#include "Proxy.h"

#define LISTENING_PORT 55555

Proxy cachingProxy;

void sig_handler(int signal) {
    cachingProxy.shutdown();
    std::cerr << "SHUTDOWN BY TERMINAL SIGNAL" << std::endl;
}

void set_sig_handler() {
    struct sigaction sig_action{};
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = sig_handler;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    sig_action.sa_mask = set;
    sigaction(SIGTERM, &sig_action, nullptr);
    sigaction(SIGINT, &sig_action, nullptr);
}

int main() {
    pollfd a[11] = {0};
    a[0].fd = -123;
    a[1].fd = -123;
    a[0].events = POLLIN;

    if (poll(a, 11, -1) == -1) {
        std::cerr << "syka";
    }

//    set_sig_handler();
//    cachingProxy = Proxy(LISTENING_PORT);
//    cachingProxy.run();
}
