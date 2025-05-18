#pragma once 

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_map>


class Env {
	Env() = default;
	std::unordered_map<std::string, std::string> envMap;
public:
	static Env& getInstance() {
		static Env instance;
		return instance;
	}
	void loadDotEnv(std::filesystem::path path = "") {
		if (path.empty()) {
			path = std::filesystem::current_path() / "../.env";
		}
		std::cout << "load .env " << path << std::endl;
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "Error: could not open .env file" << std::endl;
			return;
		}

		std::string line;
		while (std::getline(file, line)) {
			if (line.empty() || line[0] == '#') {
				continue; // Skip empty lines and comments
			}
			size_t pos = line.find('=');
			if (pos == std::string::npos) {
				continue; // Invalid line format
			}
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			envMap[key] = value;
		}
	}
	std::string getEnv(const std::string& key) {
		if (envMap.find(key) == envMap.end()) {
			throw std::runtime_error("Error: key not found in .env file");
		}
		return envMap[key];
	}
};