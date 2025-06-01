#include "socket.h"
#include "http.h"
#include <iostream>
#include <print>
#include <thread>
#include <vector>
#include <future>
#include <codecvt>
#include <nlohmann/json.hpp>
#include <stacktrace>
#include "detail.h"
#include "ollama.h"
#include <server.h>
#include <env.h>
using std::print;
using std::println;
using json = nlohmann::json;

void msvc_terminate_handler() {
	std::cout << std::stacktrace::current() << '\n';
	auto exception_ptr = std::current_exception();
	try {
		if (exception_ptr) {
			std::rethrow_exception(exception_ptr);
		}
	}
	catch (const std::exception& e) {
		println(stderr, "Unhandled exception:\ntype : {}\nwhat: {}", typeid(e).name(), e.what());
	}
	catch (...) {
		println(stderr, "Unknown exception occurred");
	}
	std::abort();
}


int main() {
    #ifdef _MSC_VER
	std::set_terminate(msvc_terminate_handler);
    #endif
	
	/*system("chcp 65001");
	system("cls");*/

	BOOL success = SetConsoleOutputCP(CP_UTF8);

	if (!success) {
		std::cerr << "Failed to set console output code page to UTF-8" << std::endl;
	}


	Server server;

	server.addHandler("chat", [](std::shared_ptr<SocketClient> clientPtr, json msg) {
		HistoryService& historyService = HistoryService::getInstance();
		OllamaClient ollamaClient(historyService.getContext(msg["content"]["id"]));
		historyService.updateTime(msg["content"]["id"]);
		json resp;
		resp["type"] = "Chunked";
		clientPtr->send(to_string(resp) + "\n");
		auto stream = ollamaClient.chatStream(msg["content"]["message"]);

		for (std::string line : stream) {
			std::cout << line << std::endl;
			json jsonLine;
			jsonLine["messageChunk"] = encodeSpecial(line);
			jsonLine["isEnd"] = false;
			std::cout << jsonLine.dump() << std::endl;
			clientPtr->send(to_string(jsonLine) + "\n");
		}
		json endMsg;
		endMsg["messageChunk"] = "";
		endMsg["isEnd"] = true;
		clientPtr->send(to_string(endMsg) + "\n");
	});

	server.addHandler("getChatHistories", [](std::shared_ptr<SocketClient> clientPtr, json msg) {
		//std::cout << "getChatHistories" << std::endl;
		HistoryService& historyService = HistoryService::getInstance();
		json response = json::array();
		for (auto& record : historyService.getHistories()) {
			response.push_back(record);
		}
		std::cout << response.dump() << std::endl;
		clientPtr->send(to_string(response) + "\n");
	});

	server.addHandler("createChat", [](std::shared_ptr<SocketClient> clientPtr, json msg) {
		HistoryService& historyService = HistoryService::getInstance();
		auto [newContext, id] = historyService.getNewContext();
		OllamaClient ollamaClient(newContext);
		ollamaClient.init("gemma3:12b-it-qat");
		json response;
		response["id"] = id;
		clientPtr->send(to_string(response) + "\n");
	});

	server.addHandler("getHistory", [](std::shared_ptr<SocketClient> clientPtr, json msg) {
		HistoryService& historyService = HistoryService::getInstance();
		auto chatContext = historyService.getContext(msg["content"]["id"]);
		json response = json::array();
		for (auto& message : (*chatContext)["messages"]) {
			//data class ChatMessage(val message: String, val sender : Sender)
			json chatMessage;
			chatMessage["message"] = message["content"];
			if (message["role"] == "user") {
				chatMessage["sender"] = "USER";
			}
			else if (message["role"] == "assistant") {
				chatMessage["sender"] = "BOT";
			}
			else {
				continue;
			}
			response.push_back(chatMessage);
		}
		clientPtr->send(to_string(response) + "\n");
	});

	server.addHandler("getOpenAiApiKey", [](std::shared_ptr<SocketClient> clientPtr, json msg) {
		clientPtr->send(Env::getInstance().getEnv("OPENAI_API_KEY") + "\n");
	});

	server.serve("0.0.0.0", 8080);

	
	while (1) {
		try {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (...) {
			std::cout << "error" << std::endl;
		}
	}
	
}