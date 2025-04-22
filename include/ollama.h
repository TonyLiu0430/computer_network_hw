#pragma once

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "http.h"
#include "ChatContext.h"
using json = nlohmann::json;

class OllamaClient {
    std::shared_ptr<ChatContext> &context;
    ChatContext& body;
public:
   OllamaClient(std::shared_ptr<ChatContext> context) : context(context), body(*context) {}

   constexpr std::vector<std::string> getModels() {
	   return { "gemma3:12b", "gemma3:12b-it-qat"};
   }

   void init(std::string model) {
	   body["model"] = "gemma3:12b-it-qat";
	   body["messages"] = json::array();
	   json systemPrompt;
	   systemPrompt["role"] = "system";
	   systemPrompt["content"] = "請使用繁體中文回覆問題，並且不需要使用 markdown 語法、回答盡量減短，語氣可愛一點。";
	   body["messages"].push_back(systemPrompt);
   }

   static std::string chatNoRecord(std::string model, std::string prompt, std::string systemPrompt = "") {
        HttpClient http;
        http.connect("127.0.0.1", 11434);
        
        json body;
        body["model"] = "gemma3:12b-it-qat";
        body["prompt"] = prompt;
        body["stream"] = false;
        if (systemPrompt != "") {
            body["system"] = systemPrompt;
        }

        auto resp = http.post("/api/generate", body);
        
        json jsonResp = json::parse(resp.body);

        return jsonResp["response"];
   }

   std::string chat(std::string prompt) {
	    HttpClient http;
        http.connect("127.0.0.1", 11434);

        std::cout << body.dump() << std::endl;
        json message;
        message["role"] = "user";
        message["content"] = prompt;
        body["messages"].push_back(message);
        std::string resp;
        auto res = http.post("/api/chat", body);
        if (res.chunkedBody) {
            std::string allResponse;
            for (std::string line : *res.chunkedBody) {
                json jsonLine = json::parse(line);
                std::cout << std::string(jsonLine["message"]["content"]);
		        resp += jsonLine["message"]["content"];
                allResponse += jsonLine["message"]["content"];
            }
            json response;
            response["role"] = "assistant";
            response["content"] = allResponse;
            body["messages"].push_back(response);
        }
        return resp;
   }

   std::generator<std::string> chatStream(std::string prompt) {
       HttpClient http;
       http.connect("127.0.0.1", 11434);

       std::cout << body.dump() << std::endl;
       json message;
       message["role"] = "user";
       message["content"] = prompt;
       body["messages"].push_back(message);
       std::string resp;
       auto res = http.post("/api/chat", body);
       if (res.chunkedBody) {
           std::string allResponse;
           for (std::string line : *res.chunkedBody) {
               json jsonLine = json::parse(line);
               co_yield (std::string)jsonLine["message"]["content"];
               allResponse += jsonLine["message"]["content"];
           }
           json response;
           response["role"] = "assistant";
           response["content"] = allResponse;
           body["messages"].push_back(response);
       }
   }
};