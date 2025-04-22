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
								HistoryService& historyService = HistoryService::getInstance();
								OllamaClient ollamaClient(historyService.getContext(msg["content"]["id"]));
								json resp;
								resp["type"] = "Chunked";
								clientPtr->send(to_string(resp) + "\n");
								auto stream = ollamaClient.chatStream(msg["content"]["message"]);

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
								for (auto& record : historyService.getHistories()) {
									response.push_back(record);
								}
								std::cout << response.dump() << std::endl;
								clientPtr->send(to_string(response) + "\n");
							}
							else if (msg["method"] == "createChat") {
								HistoryService& historyService = HistoryService::getInstance();
								auto [newContext, id] = historyService.getNewContext();
								OllamaClient ollamaClient(newContext);
								ollamaClient.init("gemma3:12b-it-qat");
								json response;
								response["id"] = id;
								clientPtr->send(to_string(response) + "\n");
							}
							else if(msg["method"] == "getHistory") {
								HistoryService& historyService = HistoryService::getInstance();
								auto chatContext = historyService.getContext(msg["content"]["id"]);
								json response = json::array();
								for (auto& message : (*chatContext)["messages"]) {
									//data class ChatMessage(val message: String, val sender : Sender)
									json chatMessage;
									chatMessage["message"] = message["content"];
									if(message["role"] == "user") {
										chatMessage["sender"] = "USER";
									}
									else if(message["role"] == "assistant") {
										chatMessage["sender"] = "BOT";
									}
									else {
										continue;
									}
									response.push_back(chatMessage);
								}
								clientPtr->send(to_string(response) + "\n");
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
};