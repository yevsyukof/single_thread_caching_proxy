cmake_minimum_required(VERSION 3.20)
project(single_thread_porxy)

set(CMAKE_CXX_STANDARD 20)

add_executable(single_thread_porxy main.cpp Proxy.cpp Proxy.h HttpParser/HttpParser.h HttpParser/HttpParser.c Connections/ClientConnection.cpp Connections/ClientConnection.h Connections/ServerConnection.cpp Connections/ServerConnection.h Cache/Cache.cpp Cache/Cache.h Cache/CacheEntry.cpp Cache/CacheEntry.h Connections/Connection.h Constants.h HttpParser/HttpRequest.h)
