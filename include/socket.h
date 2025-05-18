#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdexcept>
#include <format>
#include <print>
#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>

#define ERROR_CODE_DUMP(x) #x ## " : " + std::to_string((x)())

class WSAInitializer {
private:
    WSAInitializer() {
        WSADATA wsa = { 0 };
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != NO_ERROR) {
            throw std::runtime_error("Error: init winsock");
        }
    }

    ~WSAInitializer() {
        WSACleanup();
    }

    friend void initWSAInitializer();
};

static void initWSAInitializer() {
	static WSAInitializer instance;
}

static bool __b = []() {
	initWSAInitializer();
	return true;
}();


class Socket {
protected:
public:
    SOCKET sock;
    Socket() {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            throw std::runtime_error(std::format("Socket creation error {}", ERROR_CODE_DUMP(WSAGetLastError)));
        }
    }
public:
    Socket(SOCKET sock) : sock(sock) {}

	bool isValid() const {
		return sock != INVALID_SOCKET;
	}

    virtual ~Socket() {
        closesocket(sock);
    }
};

class SocketClient : public Socket {
protected:
    void make_nonBlocking() {
        u_long mode = 1; // non-blocking mode
        if (ioctlsocket(sock, FIONBIO, &mode)) {
            throw std::runtime_error("Error: set socket to non-blocking mode");
        }
    }
	//bool is_nonBlocking() {
	//	u_long mode = 0;
	//	if (ioctlsocket(sock, FIONBIO, &mode)) {
	//		throw std::runtime_error("Error: get socket to non-blocking mode");
	//	}
	//	return mode == 1;
	//}
public:
    SocketClient(SocketClient &&other) noexcept : Socket(other.sock) {
        other.sock = INVALID_SOCKET; // Prevent double close
    }
    SocketClient(const SocketClient& other) : Socket(other.sock) {}
    SocketClient(SOCKET sock) : Socket(sock) {}
	SocketClient() : Socket() {}

    void connect(std::string host, int port) {
        sockaddr_in serv_name{};
        int status;

        serv_name.sin_family = AF_INET;
        inet_pton(AF_INET, host.c_str(), &serv_name.sin_addr);
        serv_name.sin_port = htons(port);
        status = ::connect(sock, (sockaddr *)&serv_name, sizeof(serv_name));
        if (status == -1) {
            throw std::runtime_error(std::format("Connection error {}", ERROR_CODE_DUMP(WSAGetLastError)));
        }
    }

	//void connect(const sockaddr_in addr) {
	//	int status = ::connect(sock, (sockaddr*)&addr, sizeof(addr));
	//	if (status == -1) {
	//		throw std::runtime_error(std::format("Connection error {}", ERROR_CODE_DUMP(WSAGetLastError)));
	//	}
	//}

    void make_keep_alive(bool open = true) {
        int optval = open;
        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) == -1) {
            throw std::runtime_error("Error: set socket to keep alive mode");
        }
    }

    template<typename T>
    void send(const T &data) {
        ::send(sock, reinterpret_cast<const char*>(&data), int(sizeof(data)), 0);
    }

    template<typename T> requires std::is_same_v<T, std::string>
    void send(const T &data)  {
        ::send(sock, data.c_str(), int(data.size()), 0);
    }

    std::string receive(int buffersize = 1024) {
        std::vector<char> buffer(buffersize);
        int nbytes = recv(sock, buffer.data(), buffer.size(), 0);
		if (nbytes == 0) { //graceful shutdown
			closesocket(sock);
		}
        if (nbytes == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(sock);
            throw std::runtime_error(std::format("recv failed {}", ERROR_CODE_DUMP(WSAGetLastError)));
        }
        auto res = std::string(buffer.data(), nbytes);
		return res;
    }

	template<typename T> requires std::same_as<T, std::unique_ptr<char[]>>
	std::pair<std::unique_ptr<char[]>, int> receive(int buffersize = 1024) {
		std::unique_ptr<char[]> buffer(new char[buffersize]);
		int nbytes = recv(sock, buffer.get(), buffersize, 0);
        if (nbytes == 0) {
			closesocket(sock);
			throw std::runtime_error(std::format("recv failed {}", ERROR_CODE_DUMP(WSAGetLastError)));
		}
        if (nbytes == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(sock);
            throw std::runtime_error(std::format("recv failed {}", ERROR_CODE_DUMP(WSAGetLastError)));
        }
        if (nbytes == -1) {
			return { nullptr, 0 };
        }
        return {std::move(buffer), nbytes };
	}
};

class SocketServer : public Socket {
public:
    SocketServer() : Socket() {}
    void serve(const std::string &host, int port, int numConnections = 1024) {
        int on = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int)) == -1) {
            throw std::runtime_error("Setsockopt error");
        }

        sockaddr_in my_addr;
        my_addr.sin_family = AF_INET;
        inet_pton(AF_INET, host.c_str(), &my_addr.sin_addr);
        my_addr.sin_port = htons(port);

        int status = bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));
        if (status == -1) {
            throw std::runtime_error(std::format("failed to bind socket to address: {}:{}", host, port));
        }
        char my_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &my_addr.sin_addr, my_ip, sizeof(my_ip));

        status = ::listen(sock, numConnections);
        if (status == -1) {
            throw std::runtime_error(std::format("failed to listen on socket: {}:{}", host, port));
        }
    }

    SocketClient accept() {
        sockaddr_in client_addr{};
        int addrlen = sizeof(client_addr);
        SOCKET new_sock = ::accept(sock, (struct sockaddr *)&client_addr, &addrlen);
        if (new_sock == INVALID_SOCKET) {
            throw std::runtime_error(std::format("Error: accept failed {}", ERROR_CODE_DUMP(WSAGetLastError)));
        }

        /*char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));*/
        auto socketClient = SocketClient(new_sock);
        socketClient.make_keep_alive(1);
        return socketClient;
    }
};

class BufferedSocketClient : public SocketClient {
private:
    struct Buffer {
		std::unique_ptr<char[]> buffer;
		size_t startIndex;
		size_t size;
	};
	std::list<Buffer> buffers;
public:
    using SocketClient::SocketClient;
	std::string getline(const std::string& delimiter = "\r\n") {
        std::vector<decltype(buffers)::iterator> erasedList;
		std::string line;
		for (auto it = buffers.begin(); true; ++it) {
			if (it == buffers.end()) {
				auto [buffer, nbytes] = SocketClient::receive<std::unique_ptr<char[]>>();
				if (nbytes == 0) {
                    it = std::prev(buffers.end());
                    continue;
				}
				buffers.push_back({ std::move(buffer), 0, size_t(nbytes) });
				it = std::prev(buffers.end());
			}
			char* begin = it->buffer.get() + it->startIndex;
			char* end = it->buffer.get() + it->size;
			auto foundit = std::search(begin, end, delimiter.begin(), delimiter.end());
            if (foundit == end) { // no found
				// add whole buffer to line
                line.append(begin, end);
				erasedList.push_back(it);
            }
            else if (foundit != end) {
				line.append(begin, foundit);
				it->startIndex += foundit - begin + delimiter.size();
                if (foundit + delimiter.size() == end) { // found and no more data
                    erasedList.push_back(it);
                }
				break;
            }
		}
        for (auto& it : erasedList) {
            buffers.erase(it);
        }

		return line;
	}
    
    std::string getRemain() {
        std::string line;
		for (auto it = buffers.begin(); it != buffers.end(); ++it) {
			char* begin = it->buffer.get() + it->startIndex;
			char* end = it->buffer.get() + it->size;
			line.append(begin, end);
		}
		buffers.clear();
		return line;
    }
    
    std::string getbytes(int nReceive) {
        std::vector<decltype(buffers)::iterator> erasedList;
		std::string data;
		for (auto it = buffers.begin(); true; ++it) {
			if (it == buffers.end()) {
				auto [buffer, nbytes] = SocketClient::receive<std::unique_ptr<char[]>>();
				if (nbytes == 0) {
					it = std::prev(buffers.end());
					continue;
				}
				buffers.push_back({ std::move(buffer), 0, size_t(nbytes) });
				it = std::prev(buffers.end());
			}
			char* begin = it->buffer.get() + it->startIndex;
			char* end = it->buffer.get() + it->size;
			if (it->size - it->startIndex >= nReceive) {
				data.append(begin, begin + nReceive);
				it->startIndex += nReceive;
				break;
			}
			else if (it->size - it->startIndex < nReceive) {
				data.append(begin, end);
				nReceive -= it->size - it->startIndex;
				erasedList.push_back(it);
			}
		}
		for (auto& it : erasedList) {
			buffers.erase(it);
		}
		return data;
	}
};