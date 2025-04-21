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
public:
    Server() : socketServer() {}
    void serve(const std::string& host, int port, int numConnections = 1024) {
        socketServer.serve(host, port, numConnections);
		std::cout << "Server started at " << host << ":" << port << std::endl;
        acceptingThread = std::thread([this]() {
            while (true) {
                SocketClient client = socketServer.accept();
				std::lock_guard<std::mutex> lock(mutex);
				//clients.push_back(client);
				std::shared_ptr<SocketClient> clientPtr = std::make_shared<SocketClient>(std::move(client));
				clientThreads.push_back(std::thread([this, clientPtr]() {
					std::shared_ptr<ChatContext> chatContext = nullptr;
					while (true) {
						try {
							auto res = clientPtr->receive(1024);
	 						json msg = json::parse(res);
							std::cout << "Received message: >>>>>" << msg["method"] << std::endl;
							if(msg["method"] == "chat") {
								std::unique_ptr<OllamaClient> ollamaClient{};
								if (chatContext == nullptr) {
									HistoryService& historyService = HistoryService::getInstance();
									chatContext = historyService.getNewContext();
									ollamaClient = std::make_unique<OllamaClient>(chatContext);
									ollamaClient->init("gemma3:12b-it-qat");
								}
								else {
									ollamaClient = std::make_unique<OllamaClient>(chatContext);
								}

								json resp;
								resp["type"] = "Chunked";
								clientPtr->send(to_string(resp) + "\n");
								auto stream = ollamaClient->chatStream(msg["content"]["message"]);

								for (std::string line : stream) {
									std::cout << line << std::endl;
									for (auto& c : line) {
										if (c == '\n') {
											c = ' ';
										}
									}
									json jsonLine;
									jsonLine["messageChunk"] = line;
									jsonLine["isEnd"] = false;
									std::cout << jsonLine.dump() << std::endl;
									clientPtr->send(to_string(jsonLine) + "\n");
								}
								json endMsg;
								endMsg["messageChunk"] = "";
								endMsg["isEnd"] = true;
								clientPtr->send(to_string(endMsg) + "\n");
							}
							else if (msg["method"] == "getChatHistories") {
								std::cout << "getChatHistories" << std::endl;
								HistoryService& historyService = HistoryService::getInstance();
								json response = json::array();
								for (auto& [key, value] : historyService.chatHistories) {
									json chatHistory;
									chatHistory["id"] = key;
									chatHistory["title"] = std::to_string(key);
									response.push_back(chatHistory);
								}
								std::cout << "getChatHistories response: " << response.dump() << std::endl;
								clientPtr->send(to_string(response) + "\n");
							}
							
						}
						catch (std::exception &e) {
							std::cout << "Client disconnected " << e.what() << std::endl;
							break;
						}
					}
				}));
            }
        });
    }
};