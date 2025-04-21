#pragma once

#include "socket.h"
#include <cppcoro/task.hpp>

class SocketClientAsync : public SocketClient {
    using SocketClient::SocketClient;
    SocketClientAsync() {
        make_async();
    }
public:
    cppcoro::task<std::string> getlineAsync(const std::string& delimiter = "\r\n") {
        co_return co_await cppcoro::task_from([this, delimiter]() -> std::string {
            return this->getline(delimiter);
        });
    }
};