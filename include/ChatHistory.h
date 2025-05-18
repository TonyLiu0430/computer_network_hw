#pragma once
#include <unordered_map>
#include "ChatContext.h"

using namespace std::literals;

struct ChatRecord {
	std::shared_ptr<ChatContext> context;
	std::chrono::system_clock::time_point updataAt = std::chrono::system_clock::now();
	std::string title = "無標題";
};

class HistoryService {
	HistoryService() = default;
	std::unordered_map<int, ChatRecord> chatHistories;
	std::list<std::thread> threads;
	struct Record {
		int id;
		std::string title;
		std::chrono::system_clock::time_point updataAt;
	};

	std::string generateTitle(int id) {
		if (chatHistories.find(id) == chatHistories.end()) {
			throw std::runtime_error("Invalid chat history ID");
		}
		auto& record = chatHistories[id];
		auto& context = record.context;
		auto& body = *context;
		auto& messages = body["messages"];
		if (messages.size() <= 2) {
			return "無標題";
		}
		OllamaClient ollamaClient(context);
		auto resp = ollamaClient.chatNoRecord("幫以上對話取個標題，直接輸出");

		for(auto &c : resp) {
			if(c == '\n' || c == '\\') {
				c = ' ';
			}
		}

		return resp;
	}
public:
	static HistoryService& getInstance() {
		static HistoryService instance{};
		return instance;
	}

	std::pair<std::shared_ptr<ChatContext>, int> getNewContext() {
		auto newContext = std::make_shared<ChatContext>();
		int newId = chatHistories.size();
		chatHistories[newId] = {newContext};
		return {newContext, newId};
	}

	std::shared_ptr<ChatContext> getContext(int id) {
		if (chatHistories.find(id) != chatHistories.end()) {
			return chatHistories[id].context;
		}
		return nullptr;
	}

	void updateTime(int id) {
		if (chatHistories.find(id) != chatHistories.end()) {
			chatHistories[id].updataAt = std::chrono::system_clock::now();
		}
	}
	
	std::vector<json> getHistories() {
		std::vector<Record> records;
		for (auto& [key, value] : chatHistories) {
			records.push_back({ key, value.title, value.updataAt });
			if(value.title == "無標題") {
				threads.emplace_back([id = key, this]() {
					chatHistories[id].title = generateTitle(id);
				});
				value.title = "生成中";
			}
		}
		sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
				return a.updataAt > b.updataAt;
			});
		std::vector<json> histories;
		for (auto& record : records) {
			json history;
			history["id"] = record.id;
			history["title"] = record.title;
			histories.push_back(history);
		}
		return histories;
	}
};