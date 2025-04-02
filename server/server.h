/* Start Header
*****************************************************************/
/*!
\file server.h
\author Poh Jing Seng, 2301363
\par jingseng.poh\@digipen.edu
\date 1 Apr 2025
\brief
This file declares the game server
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

#pragma once

#ifndef __SERVER_H__
#define __SERVER_H__

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// REQUIRED!! OR DUPLICATE DEFINITION
#endif

#include "Windows.h"		// Entire Win32 API...
// #include "winsock2.h"	// ...or Winsock alone
#include "ws2tcpip.h"		// getaddrinfo()

// Tell the Visual Studio linker to include the following library in linking.
// Alternatively, we could add this file to the linker command-line parameters,
// but including it in the source code simplifies the configuration.
#pragma comment(lib, "ws2_32.lib")

#include <iostream>			// cout, cerr
#include <string>			// string
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <thread>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <deque>
#include <bitset>
#include <future>

using SESSION_ID = int;

class Server {
private:
	Server() = default;
public:

	static Server& getInstance();

	static constexpr int TICK_RATE = 120;

	static constexpr int MAX_PACKET_SIZE = 1000;

	std::mutex _stdoutMutex;

	// server stuff
	char serverIPAddr[MAX_PACKET_SIZE]{};
	int serverUdpPort{};
	int serverUdpPortBroadcast{};
	addrinfo* server_info = nullptr;
	addrinfo* udp_info = nullptr;
	std::mutex udp_info_mutex;

	// udp stuff
	SESSION_ID session_id{};
	SOCKET udp_socket{};
	SOCKET udp_socket_broadcast{};

	//std::unordered_map<SESSION_ID, sockaddr_in> udp_clients{};
	//std::mutex udp_clients_mutex{};

	std::unordered_map<SESSION_ID, decltype(std::chrono::high_resolution_clock::now())> keep_alive_map;
	std::mutex keep_alive_mutex;

	bool udpListenerRunning = true;

	// ack stuff
	std::unordered_set<SESSION_ID> ack_conn_request_clients;
	std::mutex ack_conn_request_clients_mutex;

	std::unordered_set<SESSION_ID> ack_start_game_clients;
	std::mutex ack_start_game_clients_mutex;

	//std::unordered_set<SESSION_ID> ack_all_entities_clients;
	//std::mutex ack_all_entities_clients_mutex;

	std::unordered_set<SESSION_ID> ack_end_game_clients;
	std::mutex ack_end_game_clients_mutex;

	std::unordered_map<SESSION_ID, float> client_last_request_time;		// used to timeout client connection
	std::mutex client_last_request_time_mutex;

	// timeout stuff
	static constexpr int DISCONNECTION_TIMEOUT_DURATION_MS = 15000;
	static constexpr int TIMEOUT_MS = 200;		// timeout before retrying

	// recv stuff
	static constexpr int MAX_PACKET_QUEUE = 100;
	std::deque<std::pair<sockaddr_in, std::vector<char>>> recvbuffer_queue;
	std::mutex recvbuffer_queue_mutex;


	enum CLIENT_REQUESTS {
		CONN_REQUEST = 0,
		ACK_CONN_REQUEST,
		REQ_START_GAME,
		ACK_START_GAME,
		SELF_SPACESHIP,
		NEW_BULLET,
		ACK_END_GAME,
		KEEP_ALIVE
	};

	enum SERVER_MSGS {
		CONN_ACCEPTED = 0,
		CONN_REJECTED,
		START_GAME,
		ACK_NEW_BULLET,
		ALL_ENTITIES,
		END_GAME,
	};

	struct Client {
		int sessionId;

		std::string ip;
		std::string port;

		char cip[4];
		int iport;
	};
	std::unordered_map<SOCKET, Client> conns;
	std::mutex conns_mutex;

	template <typename T>
	static std::vector<char> t_to_bytes(T num) {
		std::vector<char> bytes(sizeof(T));

		if constexpr (std::is_integral<T>::value) {
			// for ints, convert to network byte order
			num = htonl(num);
			std::memcpy(bytes.data(), &num, sizeof(T));
		}
		else if constexpr (std::is_floating_point<T>::value) {
			// For floats
			unsigned int int_rep = *reinterpret_cast<unsigned int*>(&num);  // reinterpret float as unsigned int
			int_rep = htonl(int_rep);
			std::memcpy(bytes.data(), &int_rep, sizeof(T));
		}

		return bytes;
	}

	static float btof(const std::vector<char>& bytes) {
		if (bytes.size() != sizeof(float)) {
			throw std::invalid_argument("Invalid byte size for float conversion");
		}

		unsigned int int_rep;
		std::memcpy(&int_rep, bytes.data(), sizeof(float));
		int_rep = ntohl(int_rep);  // Convert from network byte order

		float num;
		std::memcpy(&num, &int_rep, sizeof(float));
		return num;
	}


	/**
	 * send data with udp.
	 *
	 * \param buffer
	 * \param dest_addr
	 * \return
	 */
	//int sendData(const std::vector<char>& buffer, SESSION_ID sid);

	int sendData(const std::vector<char>& buffer, sockaddr_in udp_addr_in);

	int broadcastData(const std::vector<char>& buffer);

	int getSessionId();

	SOCKET createUdpSocket(int port, bool broadcast = false);

	/**
	 * worker thread. should only be used in 1 thread in any given time.
	 *
	 */
	void udpListener();

	void requestHandler();

	int init();

	void cleanup();


	static constexpr int KEEP_ALIVE_TIMEOUT_MS = 5000;
	void keepAliveChecker();


	static std::deque<std::future<void>> threadpool;
	static std::mutex threadpool_mutex;

	static void threadpoolManager();
};



#endif // __SERVER_H__
