#pragma once
#include <unordered_map>
#include "ChatContext.h"

class HistoryService {
	HistoryService() = default;
public:
	std::unordered_map<int, std::shared_ptr<ChatContext>> chatHistories;

	static HistoryService& getInstance() {
		static HistoryService instance{};
		return instance;
	}

	std::shared_ptr<ChatContext> getNewContext() {
		auto newContext = std::make_shared<ChatContext>();
		chatHistories[chatHistories.size()] = newContext;
		return newContext;
	}
};