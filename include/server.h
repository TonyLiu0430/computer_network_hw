#pragma once

#include "socket.h"

#include "util.h"
#include <format>
#include <iostream>
#include <thread>
#include <mutex>
#include "ollama.h"
#include "ChatContext.h"
#include "ChatHistory.h"


class Server {
public:
    SocketServer socketServer;
    std::thread acceptingThread;
	std::mutex mutex;
	std::vector<SocketClient> clients;
	std::vector<std::thread> clientThreads;
	std::map<std::string, std::function<void(std::shared_ptr<SocketClient>, json)>> handlers;
public:
    Server() : socketServer() {}
    void serve(const std::string& host, int port, int numConnections = 1024) {
        socketServer.serve(host, port, numConnections);
		std::cout << "Server started at " << host << ":" << port << std::endl;
        acceptingThread = std::thread([this]() {
            while (true) {
                SocketClient client = socketServer.accept();
				std::lock_guard<std::mutex> lock(mutex);
				std::shared_ptr<SocketClient> clientPtr = std::make_shared<SocketClient>(std::move(client));
				clientThreads.push_back(std::thread([this, clientPtr]() {
					while (true) {
						try {
							auto res = clientPtr->receive(1024 * 8);
							if (res.empty()) {
								std::cout << "Client disconnected" << std::endl;
								break;
							}
	 						json msg = json::parse(res);
							std::cout << "Received message: >>>>>" << msg["method"] << std::endl;
							if (handlers.contains(msg["method"])) {
								handlers[msg["method"]](clientPtr, msg);
							}
						}
						catch (std::exception &e) {
							std::cout << std::stacktrace::current() << '\n';
							std::cout << "Client disconnected " << e.what() << std::endl;
							break;
						}
					}
				}));
            }
        });
	}

	void addHandler(const std::string& method, std::function<void(std::shared_ptr<SocketClient>, json)> handler) {
		handlers[method] = handler;
	}
};