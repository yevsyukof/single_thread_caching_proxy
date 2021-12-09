#include <string>
#include "ClientConnection.h"
#include "../HttpParser/HttpRequest.h"


ClientConnection::ClientConnection(int connectionSocketFd, int inPollListIdx)
        : Connection(connectionSocketFd, inPollListIdx),
          connectionState(ClientConnectionState::WAIT_FOR_REQUEST) {}


HttpRequest parseHttpRequest(std::vector<char> &request, std::string &newRequest) {
    HttpRequest buildingHttpRequest;

    const char *path;
    const char *method;
    int version;
    size_t methodLen, pathLen;

    buildingHttpRequest.headersCount = sizeof(buildingHttpRequest.headers) / sizeof(buildingHttpRequest.headers[0]);
    int reqSize = phr_parse_request(request.data(), request.size(), &method, &methodLen,
                                    &path, &pathLen, &version, buildingHttpRequest.headers,
                                    &buildingHttpRequest.headersCount, 0);

    if (reqSize == -1) {
        perror("BAD HTTP REQUEST\n");
    }

    std::string onlyPath = path;
    onlyPath.erase(onlyPath.begin() + pathLen, onlyPath.end());
    buildingHttpRequest.url = onlyPath;

    std::string onlyMethod = method;
    onlyMethod.erase(onlyMethod.begin() + methodLen, onlyMethod.end());
    buildingHttpRequest.method = onlyMethod;

    newRequest = onlyMethod + std::string(" ").append(onlyPath).append(" HTTP/1.0") + "\r\n";


    for (int i = 0; i < buildingHttpRequest.headersCount; ++i) {
        std::string headerName = buildingHttpRequest.headers[i].name;
        headerName.erase(headerName.begin() + buildingHttpRequest.headers[i].name_len, headerName.end());
        std::string headerValue = buildingHttpRequest.headers[i].value;
        headerValue.erase(headerValue.begin() + buildingHttpRequest.headers[i].value_len, headerValue.end());

        if (headerName == "Connection") {
            continue;
        }
        if (headerName == "Host") {
            buildingHttpRequest.host = headerValue;
        }

        newRequest.append(headerName).append(": ").append(headerValue) += "\r\n";
    }

    newRequest += "\r\n\r\n";

    return buildingHttpRequest;
}

int ClientConnection::receiveData() {
    return 0;
}
