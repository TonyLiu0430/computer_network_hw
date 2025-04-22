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
	
	/*detail::inner_t<std::vector<int>, std::vector> v = 10;
	std::cout << v << std::endl;
	return 0;*/
	system("chcp 65001");
	system("cls");

	// OllamaClient ollamaClient;

	// std::cout << ollamaClient.chat("hi") << std::endl;


	Server server;

	server.serve("0.0.0.0", 8080);
	
	while (1) {
		try {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (...) {
			std::cout << "error" << std::endl;
		}
	}

	


	//throw std::runtime_error("test error");

	//HttpClient http;
	//http.connect("127.0.0.1", 11434);

	//json body;
	//body["model"] = "gemma3:12b";
	//
	//body["messages"] = json::array();

	//json systemPrompt;
	//systemPrompt["role"] = "system";
	//systemPrompt["content"] = "請使用繁體中文回覆問題，並且不需要使用 markdown 語法。";
	//body["messages"].push_back(systemPrompt);

 //  std::cout << body.dump() << std::endl;

	//std::string prompt;
	//while (1) {
	//	std::cout << "\n>>>>";
	//	std::cin >> prompt;

	//	json message;
	//	message["role"] = "user";
	//	message["content"] = prompt;
	//	body["messages"].push_back(message);

	//	auto res = http.post("/api/chat", body);
	//	if (res.chunkedBody) {
	//		std::string allResponse;
	//		for (std::string line : *res.chunkedBody) {
	//			json jsonLine = json::parse(line);
	//			std::cout << std::string(jsonLine["message"]["content"]);
	//			allResponse += jsonLine["message"]["content"];
	//		}
	//		json response;
	//		response["role"] = "assistant";
	//		response["content"] = allResponse;
	//		body["messages"].push_back(response);
	//	}
	//}
	
	
}