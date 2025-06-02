#pragma once
#include "socket.h"
#include "util.h"
#include <format>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <generator>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


class HttpResponse {
public:
	std::unordered_map<std::string, std::string> headers;
	std::string body;
	int statusCode = 0;
	std::string statusMessage;
	std::string httpVersion;
	std::unique_ptr<std::generator<std::string>> chunkedBody = nullptr;
	void dump() {
		std::println("{} {} {}", httpVersion, statusCode, statusMessage);
		for (const auto& header : headers) {
			std::println("{}: {}", header.first, header.second);
		}
		std::println("");
		std::cout << body << std::endl;
	}
};


class HttpClient {
	BufferedSocketClient socketClient;
	std::string host;
	bool connected = false;
public:
	void connect(const std::string& host, int port) {
		if (isIp(host)) {
			socketClient.connect(host, port);
			this->host = std::format("{}:{}", host, port);
		}
		else { //domain
			std::string ip = getIpFromDNS(host);
			if (ip.empty()) {
				throw std::runtime_error(std::format("Error: failed to resolve host {}", host));
			}
			socketClient.connect(ip, port);
			this->host = host;
		}
		connected = true;
	}

	static std::string generateRequestHeader(std::string method, std::string uri, std::string host, const std::unordered_map<std::string, std::string> &headers = {}) {
		std::string request = std::format(
			"{} {} HTTP/1.1\r\n"
			"HOST: {}\r\n", method, uri, host);
		for (const auto &[key, value] : headers) {
			request += std::format("{}: {}\r\n", key, value);
		}
		return request + "\r\n";
	}

	HttpResponse getResponse() {
		HttpResponse httpResponse;

		std::string responseLine = socketClient.getline("\r\n");

		std::istringstream iss(responseLine);
		iss >> httpResponse.httpVersion >> httpResponse.statusCode >> httpResponse.statusMessage;

		httpResponse.headers = readHeader();
		//Content-Type: application/x-ndjson
		if (httpResponse.headers["Content-Type"] == "application/x-ndjson" && httpResponse.headers["Transfer-Encoding"] == "chunked") {
			httpResponse.chunkedBody = std::make_unique<std::generator<std::string>>(readChunkedAsyncEnumable());
		}
		else if (httpResponse.headers["Transfer-Encoding"] == "chunked") {
			httpResponse.body = readChunked();
		}
		else if (not httpResponse.headers["Content-Length"].empty()) {
			std::string contentLengthStr = httpResponse.headers["Content-Length"];
			if (!contentLengthStr.empty()) {
				int contentLength = std::stoi(contentLengthStr, nullptr, 10);
				httpResponse.body = socketClient.getbytes(contentLength);
			}
		}
		else {
			throw std::runtime_error("Error: no Content-Length or Transfer-Encoding header found");
		}

		return httpResponse;
	}

	HttpResponse get(const std::string& path) {
		std::string uri = path;
		std::string request = generateRequestHeader("GET", uri, host);//"GET / HTTP/1.1\r\nHOST: www.google.com\r\n\r\n";//generateRequest("GET", uri, host, port);
		socketClient.send(request);
		
		return getResponse();
	}

	HttpResponse post(const std::string& path, const std::string& body = "", std::unordered_map<std::string, std::string>&& header = {}) {
		std::string uri = path;

		header["Content-Type"] = "application/json";
		header["Content-Length"] = std::to_string(body.size());
		std::string request = generateRequestHeader("POST", uri, host, header);
		request += body;
		socketClient.send(request);

		return getResponse();
	}

	HttpResponse post(const std::string& path, json jsonObject) {
		std::string uri = path;
		std::unordered_map<std::string, std::string> header;
		header["Content-Type"] = "application/json";
		std::string bodyStr = to_string(jsonObject);
		header["Content-Length"] = std::to_string(bodyStr.size());
		return post(uri, bodyStr, std::move(header));
	}

private:
	std::unordered_map<std::string, std::string> readHeader() {
		std::unordered_map<std::string, std::string> headers;
		std::string line;
		while (true) {
			line = socketClient.getline();
			if (line.empty()) { // "\r\n"
				break;
			}
			//std::println("{}", line);

			std::string headerName;
			std::string headerValue;
			for (int i = 0; i < line.size(); i++) {
				if (line[i] == ':') {
					headerName = line.substr(0, i);
					headerValue = trimLeft(line.substr(i + 1));
					break;
				}
			}
			if (not headerName.empty()) {
				headers[headerName] = headerValue;
			}
		}
		return headers;
	}
	std::string readChunked() {
		std::string body;
		while (1) {
			std::string blockSizeStr = socketClient.getline();
			int blockSize = std::stoi(blockSizeStr, nullptr, 16);
			if (blockSize == 0) {
				return body;
			}
			std::string block = socketClient.getline();

			body += block;
		}
	}
	std::generator<std::string> readChunkedAsyncEnumable() {
		while (1) {
			std::string blockSizeStr = socketClient.getline();
			int blockSize = std::stoi(blockSizeStr, nullptr, 16);
			if (blockSize == 0) {
				break;
			}
			std::string line = socketClient.getline();
			co_yield line;
		}
	}
	static bool isIp(const std::string& host) {
		sockaddr_in sa{};
		int result = inet_pton(AF_INET, host.c_str(), &(sa.sin_addr));
		return result != 0;
	}
	static std::string getIpFromDNS(const std::string& host) {
		//getaddrinfo
		addrinfo hints{};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;
		addrinfo* result = nullptr;
		int status = getaddrinfo(host.c_str(), nullptr, &hints, &result);
		if (status != 0) {
			return "";
		}
		char ip[INET_ADDRSTRLEN];
		for (addrinfo* p = result; p != nullptr; p = p->ai_next) {
			if (p->ai_family == AF_INET) {
				sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
				inet_ntop(AF_INET, &(ipv4->sin_addr), ip, sizeof(ip));
				freeaddrinfo(result);
				return std::string(ip);
			}
		}
		return "";
	}
};