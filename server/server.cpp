/* Start Header
*****************************************************************/
/*!
\file server.cpp
\author Poh Jing Seng, 2301363
\par jingseng.poh\@digipen.edu
\date 27 Mar 2025
\brief
This file implements the server
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

/*******************************************************************************
 * A multi-threaded TCP/IP server application with non-blocking sockets
 ******************************************************************************/

#define VERBOSE_LOGGING

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
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

using SESSION_ID = int;

constexpr int MAX_PACKET_SIZE = 1000;

std::mutex _stdoutMutex;

// server stuff
char serverIPAddr[MAX_PACKET_SIZE];
int serverUdpPort{};
int serverUdpPortBroadcast{};
addrinfo* udp_info = nullptr;
std::mutex udp_info_mutex;

// udp stuff
SESSION_ID session_id{};
SOCKET udp_socket{};
SOCKET udp_socket_broadcast{};

std::unordered_map<SESSION_ID, sockaddr_in> udp_clients;
std::mutex udp_clients_mutex;

bool udpListenerRunning = true;

// ack stuff
std::unordered_set<SESSION_ID> ack_start_game_clients;
std::mutex ack_start_game_clients_mutex;

std::unordered_set<SESSION_ID> ack_conn_request_clients;
std::mutex ack_conn_request_clients_mutex;

std::unordered_set<SESSION_ID> ack_self_spaceship_clients;
std::mutex ack_self_spaceship_clients_mutex;

std::unordered_set<SESSION_ID> ack_all_entities_clients;
std::mutex ack_all_entities_clients_mutex;

std::unordered_set<SESSION_ID> ack_end_game_clients;
std::mutex ack_end_game_clients_mutex;

std::unordered_map<SESSION_ID, float> client_last_request_time;		// used to timeout client connection
std::mutex client_last_request_time_mutex;

std::vector<SESSION_ID> active_sessions;
std::mutex active_sessions_mutex;

// timeout stuff
constexpr int DISCONNECTION_TIMEOUT_DURATION_MS = 15000;
constexpr int TIMEOUT_MS = 200;		// timeout before retrying

// recv stuff
constexpr int MAX_PACKET_QUEUE = 100;
std::deque<std::pair<sockaddr_in, std::vector<char>>> recvbuffer_queue;
std::mutex recvbuffer_queue_mutex;

// game stuff
constexpr int MAX_PLAYERS = 4;

// for ALL_ENTITIES payload
struct vec2 {
	union {
		struct { float x, y; };
		float raw[2];
	};
};


struct Asteroid {
	vec2 pos;
	vec2 vector;
};

struct Bullet : public Asteroid {
	SESSION_ID sid;
};

struct Spaceship : public Bullet {
	float rotation;
	int lives_left;
};

class Game {
public:
	std::vector<Spaceship> spaceships;
	std::vector<Bullet> bullets;
	std::vector<Asteroid> asteroids;

	std::vector<char> toBytes();
};


void emptyfn() {}

enum CLIENT_REQUESTS {
	CONN_REQUEST = 0,
	ACK_CONN_REQUEST,
	REQ_START_GAME,
	ACK_START_GAME,
	ACK_ACK_SELF_SPACESHIP,
	SELF_SPACESHIP,
	ACK_ALL_ENTITIES,
	ACK_END_GAME
};

enum SERVER_MSGS {
	CONN_ACCEPTED = 0,
	CONN_REJECTED,
	START_GAME,
	ACK_SELF_SPACESHIP,
	ALL_ENTITIES,
	END_GAME
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
std::vector<char> to_bytes(T num) {
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


std::vector<char> Game::toBytes() {
	std::vector<char> buf;

	// num spaceships
	buf.push_back((char)spaceships.size());
	std::vector<char> bytes;

	for (const Spaceship& s : spaceships) {
		// only stuff integral to rendering/interaction is sent
		// vector is not sent

		buf.push_back(s.sid & 0xff);		// spaceship sid

		bytes = to_bytes(s.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = to_bytes(s.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)

		bytes = to_bytes(s.rotation);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// rotation in degrees (4 bytes)

		buf.push_back(s.lives_left);						// lives left (1 byte)
	}

	// num bullets
	buf.push_back((char)bullets.size());

	for (const Bullet& b : bullets) {
		// vector is not sent, not integral to rendering

		buf.push_back(b.sid & 0xff);		// bullet sid

		bytes = to_bytes(b.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = to_bytes(b.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)
	}

	// num asteroids
	buf.push_back((char)asteroids.size());

	for (const Asteroid& a : asteroids) {
		// vector not sent, not needed for rendering

		bytes = to_bytes(a.pos.x);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos x (4 bytes)

		bytes = to_bytes(a.pos.y);
		buf.insert(buf.end(), bytes.begin(), bytes.end());	// pos y (4 bytes)
	}

	return buf;
}


/**
 * send data with udp.
 *
 * \param buffer
 * \param dest_addr
 * \return
 */
int sendData(const std::vector<char>& buffer, SESSION_ID sid) {
	sockaddr_in* udp_addr_in = nullptr;
	{
		std::lock_guard<std::mutex> usersLock{ udp_clients_mutex };

		if (udp_clients.find(sid) == udp_clients.end()) {
			//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };		// potential deadlock
			//std::cerr << "Session ID not found" << std::endl;
			return SOCKET_ERROR;
		}

		udp_addr_in = reinterpret_cast<sockaddr_in*>(&udp_clients.at(sid));
	}

	if (udp_addr_in == nullptr) {
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "udp_addr_in is nullptr" << std::endl;
		return SOCKET_ERROR;
	}

	if (udp_socket == INVALID_SOCKET) {
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Invalid socket." << std::endl;
		return SOCKET_ERROR;
	}

	int bytesSent = sendto(udp_socket, buffer.data(), (int)buffer.size(), 0, reinterpret_cast<sockaddr*>(&const_cast<sockaddr_in&>(*udp_addr_in)), sizeof(sockaddr_in));
	if (bytesSent == SOCKET_ERROR) {
		//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		char errorBuffer[256];
		strerror_s(errorBuffer, sizeof(errorBuffer), errno);
		//std::cerr << "sendto() failed: " << errorBuffer << std::endl;

		int wsaError = WSAGetLastError();
		//std::cerr << "sendto() failed with wsa error: " << wsaError << std::endl;
	}
	return bytesSent;
}

int sendData(const std::vector<char>& buffer, sockaddr_in udp_addr_in) {
	if (udp_socket == INVALID_SOCKET) {
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Invalid socket." << std::endl;
		return SOCKET_ERROR;
	}

	int bytesSent = sendto(udp_socket, buffer.data(), (int)buffer.size(), 0, reinterpret_cast<sockaddr*>(&udp_addr_in), sizeof(sockaddr_in));
	if (bytesSent == SOCKET_ERROR) {
		//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		char errorBuffer[256];
		strerror_s(errorBuffer, sizeof(errorBuffer), errno);
		//std::cerr << "sendto() failed: " << errorBuffer << std::endl;

		int wsaError = WSAGetLastError();
		//std::cerr << "sendto() failed with wsa error: " << wsaError << std::endl;
	}
	return bytesSent;
}

int broadcastData(const std::vector<char>& buffer) {
	if (udp_socket_broadcast == INVALID_SOCKET) {
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Invalid socket." << std::endl;
		return SOCKET_ERROR;
	}

	// Prepare the broadcast address
	sockaddr_in udp_addr_in;
	SecureZeroMemory(&udp_addr_in, sizeof(udp_addr_in));
	udp_addr_in.sin_family = AF_INET;
	udp_addr_in.sin_port = htons(serverUdpPortBroadcast);  // Make sure to use the correct port
	udp_addr_in.sin_addr.s_addr = htonl(INADDR_BROADCAST);  // Broadcast address 255.255.255.255

	int bytesSent = sendto(udp_socket_broadcast, buffer.data(), (int)buffer.size(), 0, reinterpret_cast<sockaddr*>(&udp_addr_in), sizeof(sockaddr_in));
	if (bytesSent == SOCKET_ERROR) {
		//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		char errorBuffer[256];
		strerror_s(errorBuffer, sizeof(errorBuffer), errno);
		//std::cerr << "sendto() failed: " << errorBuffer << std::endl;

		int wsaError = WSAGetLastError();
		//std::cerr << "sendto() failed with wsa error: " << wsaError << std::endl;
	}
	return bytesSent;
}

int getSessionId() {
	return session_id++;
}

SOCKET createUdpSocket(int port, bool broadcast = false) {
	// Create a UDP socket
	addrinfo hints{};
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_DGRAM;		// UDP (datagram)
	hints.ai_protocol = IPPROTO_UDP;	// UDP
	hints.ai_flags = AI_PASSIVE;

	std::lock_guard<std::mutex> usersLock{ udp_info_mutex };

	std::string udpPortString = std::to_string(port);
	int errorCode = getaddrinfo(nullptr, udpPortString.c_str(), &hints, &udp_info);
	if ((errorCode) || (udp_info == nullptr))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}

	SOCKET udpSocket = socket(
		udp_info->ai_family,
		udp_info->ai_socktype,
		udp_info->ai_protocol);
	if (udpSocket == INVALID_SOCKET)
	{
		std::cerr << "socket() failed." << std::endl;
		freeaddrinfo(udp_info);
		WSACleanup();
		return INVALID_SOCKET;
	}

	if (broadcast && setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&broadcast), sizeof(broadcast)) == SOCKET_ERROR)
	{
		std::cerr << "setsockopt() failed to enable broadcasting." << std::endl;
		closesocket(udpSocket);
		freeaddrinfo(udp_info);
		WSACleanup();
		return INVALID_SOCKET;
	}

	errorCode = bind(
		udpSocket,
		udp_info->ai_addr,
		static_cast<int>(udp_info->ai_addrlen));
	if (errorCode != NO_ERROR)
	{
		std::cerr << "bind() failed." << std::endl;
		closesocket(udpSocket);
		udpSocket = INVALID_SOCKET;
	}
	if (udpSocket == INVALID_SOCKET)
	{
		std::cerr << "bind() failed." << std::endl;
		WSACleanup();
		return INVALID_SOCKET;
	}

	// get assigned port number
	sockaddr_in udpAddr{};
	socklen_t addrLen = sizeof(udpAddr);
	int result_port;
	if (getsockname(udpSocket, (sockaddr*)&udpAddr, &addrLen) == 0) {
		result_port = ntohs(udpAddr.sin_port); // Convert from network byte order
	}
	else {
		std::cerr << "getsockname() failed." << std::endl;
		closesocket(udpSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	//freeaddrinfo(udp_info);
	return udpSocket;
}

/**
 * worker thread. should only be used in 1 thread in any given time.
 *
 */
void udpListener() {
	if (udp_socket == INVALID_SOCKET) {
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Invalid socket." << std::endl;
		return;
	}

	std::vector<char> recvbuffer;
	recvbuffer.resize(MAX_PACKET_SIZE);

	while (udpListenerRunning) {
		sockaddr_in senderAddr;
		socklen_t senderAddrLen = sizeof(senderAddr);

		// Receive data from any client
		int bytesReceived = recvfrom(udp_socket, recvbuffer.data(), (int)recvbuffer.size(), 0,
			reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);

		if (bytesReceived == -1 && WSAGetLastError() == WSAEWOULDBLOCK) {
			// non blocking socket, no data to receive
			continue;
		}
		if (bytesReceived == -1 && WSAGetLastError() == WSAECONNRESET) {
			// client crashed?
		}
		else if (bytesReceived == -1) {
			std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
			char errorBuffer[256];
			strerror_s(errorBuffer, sizeof(errorBuffer), errno);
			std::cerr << "recvfrom() failed: " << errorBuffer << std::endl;

			int wsaError = WSAGetLastError();
			std::cerr << "recvfrom() failed with wsa error: " << wsaError << std::endl;
		}

		// acks
		const int cmd = recvbuffer[0];
		const int sid = recvbuffer[1];
		bool isAck = false;

		switch (cmd) {
		case ACK_CONN_REQUEST: {
			isAck = true;
			std::lock_guard<std::mutex> acklock(ack_conn_request_clients_mutex);
			ack_conn_request_clients.insert(sid);
			break;
		}
		case ACK_START_GAME: {
			isAck = true;
			std::lock_guard<std::mutex> acklock(ack_start_game_clients_mutex);
			ack_start_game_clients.insert(sid);
			break;
		}
		case ACK_ACK_SELF_SPACESHIP: {
			isAck = true;
			std::lock_guard<std::mutex> acklock(ack_self_spaceship_clients_mutex);
			ack_self_spaceship_clients.insert(sid);
			break;
		}
		case ACK_ALL_ENTITIES: {
			isAck = true;
			std::lock_guard<std::mutex> acklock(ack_all_entities_clients_mutex);
			ack_all_entities_clients.insert(sid);
			break;
		}
		case ACK_END_GAME: {
			isAck = true;
			std::lock_guard<std::mutex> acklock(ack_end_game_clients_mutex);
			ack_end_game_clients.insert(sid);
			break;
		}
		}

		if (isAck) {
			// if is ack, dont push into recvbuffer_queue as already recorded in ack
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(recvbuffer_queue_mutex);
			recvbuffer_queue.push_back({ senderAddr, recvbuffer });
		}
	}
}


void requestHandler() {

	while (udpListenerRunning) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		// get data
		std::deque<std::pair<sockaddr_in, std::vector<char>>> recvbuffer;
		{
			std::lock_guard<std::mutex> lock(recvbuffer_queue_mutex);

			recvbuffer = recvbuffer_queue;
			recvbuffer_queue.clear();
		}

		static std::vector<char> sbuf(MAX_PACKET_SIZE);
		if (sbuf.size() != MAX_PACKET_SIZE)
			sbuf.resize(MAX_PACKET_SIZE);

		// handle data
		for (const auto [senderAddr, rbuf] : recvbuffer) {
			const int cmd = rbuf[0];

			switch (cmd) {
			case CONN_REQUEST: {
				if (udp_clients.size() >= MAX_PLAYERS) {
					sbuf[0] = CONN_REJECTED;
					sendData(sbuf, senderAddr);
					break;
				}

				const int sid = getSessionId();

				{
					std::lock_guard<std::mutex> active_sessions_lock(active_sessions_mutex);
					active_sessions.push_back(sid);
				}

				int buf_idx{};

				sbuf[buf_idx++] = CONN_ACCEPTED;

				// session id
				sbuf[buf_idx++] = sid;

				// broadcast port
				sbuf[buf_idx++] = (serverUdpPortBroadcast >> 8) & 0xff;
				sbuf[buf_idx++] = serverUdpPortBroadcast & 0xff;

				// spawn locations (world pos)
				constexpr float spawnX = 0;
				constexpr float spawnY = 0;

				memcpy(sbuf.data() + buf_idx, to_bytes(spawnX).data(), sizeof(float));
				buf_idx += (int)sizeof(float) / 8;
				memcpy(sbuf.data() + buf_idx, to_bytes(spawnY).data(), sizeof(float));
				buf_idx += (int)sizeof(float) / 8;

				// spawn rotation
				constexpr int rotation_deg = 0;

				sbuf[buf_idx++] = (rotation_deg >> 24) & 0xff;
				sbuf[buf_idx++] = (rotation_deg >> 16) & 0xff;
				sbuf[buf_idx++] = (rotation_deg >> 8) & 0xff;
				sbuf[buf_idx++] = rotation_deg & 0xff;

				// num lives
				constexpr int starting_lives = 3;

				sbuf[buf_idx++] = starting_lives;

				sendData(sbuf, senderAddr);

				break;
			}
			case REQ_START_GAME: {
				std::vector<char> buf(MAX_PACKET_SIZE);
				buf.resize(MAX_PACKET_SIZE);

				buf[0] = START_GAME;

				int num_conns{};
				{
					std::lock_guard<std::mutex> lock(active_sessions_mutex);
					num_conns = active_sessions.size();
				}

				auto bc = [&buf, num_conns]() {
					int num_acks{};

					auto start = std::chrono::high_resolution_clock::now();

					// wait for all clients to ack
					while (num_acks < num_conns) {
						auto curr = std::chrono::high_resolution_clock::now();

						if (curr - start >= std::chrono::milliseconds(DISCONNECTION_TIMEOUT_DURATION_MS)) {
							// disconnect clients that did not ack
							std::unordered_set<SESSION_ID> acked_clients;
							{
								std::lock_guard<std::mutex> req_start_game_ack_lock(ack_start_game_clients_mutex);
								acked_clients = ack_start_game_clients;
							}


							{
								std::unordered_set<SESSION_ID> timeout_disconnect_clients;
								std::lock_guard<std::mutex> clientslock(active_sessions_mutex);

								// remove sessions that timed out
								for (auto it = active_sessions.begin(); it != active_sessions.end();) {
									if (acked_clients.find(*it) != acked_clients.end()) {
										++it;
										continue;
									}
									it = active_sessions.erase(it);
								}
							}

							break;
						}

						broadcastData(sbuf);

						std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));

						{
							std::lock_guard<std::mutex> acklock(ack_start_game_clients_mutex);
							num_acks = (int)ack_start_game_clients.size();
						}
					}

					// cleanup acks
					{
						std::lock_guard<std::mutex> req_start_game_ack_lock(ack_start_game_clients_mutex);
						ack_start_game_clients.clear();
					}
					};

				// broadcast game start through reliable udp communication
				bc();

				break;
			}
			case SELF_SPACESHIP: {
				
			}
			}
		}
	}
}

int main()
{
	std::string udpPortString;

#define JS_DEBUG
#ifndef JS_DEBUG
	{
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cout << "Server UDP port number: ";
	}
	std::getline(std::cin, udpPortString);
	serverUdpPort = std::stoi(udpPortString);
#else
	udpPortString = "3001";
	serverUdpPort = 3001;
#endif
	serverUdpPortBroadcast = serverUdpPort + 1;


	// -------------------------------------------------------------------------
	// Start up Winsock, asking for version 2.2.
	//
	// WSAStartup()
	// -------------------------------------------------------------------------

	// This object holds the information about the version of Winsock that we
	// are using, which is not necessarily the version that we requested.
	WSADATA wsaData{};

	// Initialize Winsock. You must call WSACleanup when you are finished.
	// As this function uses a reference counter, for each call to WSAStartup,
	// you must call WSACleanup or suffer memory issues.
	int errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errorCode != NO_ERROR)
	{
		std::cerr << "WSAStartup() failed." << std::endl;
		return errorCode;
	}


	// -------------------------------------------------------------------------
	// Resolve own host name into IP addresses (in a singly-linked list).
	//
	// getaddrinfo()
	// -------------------------------------------------------------------------

	// Object hints indicates which protocols to use to fill in the info.
	addrinfo hints{};
	SecureZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			// IPv4
	// For UDP use SOCK_DGRAM instead of SOCK_STREAM.
	hints.ai_socktype = SOCK_DGRAM;	// Reliable delivery
	// Could be 0 for autodetect, but reliable delivery over IPv4 is always TCP.
	hints.ai_protocol = IPPROTO_UDP;
	// Create a passive socket that is suitable for bind() and listen().
	hints.ai_flags = AI_PASSIVE;

	// Get local hostname
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) != 0) {
		std::cerr << "gethostname() failed." << std::endl;
		WSACleanup();
		return 1;
	}

	addrinfo* info = nullptr;
	errorCode = getaddrinfo(hostname, udpPortString.c_str(), &hints, &info);
	if ((errorCode) || (info == nullptr))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}

	/* PRINT SERVER IP ADDRESS AND PORT NUMBER */
	struct sockaddr_in* serverAddress = reinterpret_cast<struct sockaddr_in*> (info->ai_addr);
	inet_ntop(AF_INET, &(serverAddress->sin_addr), serverIPAddr, INET_ADDRSTRLEN);
	getnameinfo(info->ai_addr, static_cast <socklen_t> (info->ai_addrlen), serverIPAddr, sizeof(serverIPAddr), nullptr, 0, NI_NUMERICHOST);
	{
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cout << std::endl;
		std::cout << "Server IP Address: " << serverIPAddr << std::endl;
		std::cout << "Server UDP Port Number: " << serverUdpPort << std::endl;
		std::cout << "Server UDP broadcast Port Number: " << serverUdpPortBroadcast << std::endl;
	}

	// create UDP socket
	udp_socket = createUdpSocket(serverUdpPort);
	if (udp_socket == INVALID_SOCKET) {
		closesocket(udp_socket);
		udp_socket = INVALID_SOCKET;
		WSACleanup();
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Failed to create UDP socket" << std::endl;
		return 4;
	}

	udp_socket_broadcast = createUdpSocket(serverUdpPortBroadcast, true);
	if (udp_socket == INVALID_SOCKET) {
		closesocket(udp_socket);
		udp_socket = INVALID_SOCKET;
		WSACleanup();
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Failed to create UDP socket" << std::endl;
		return 4;
	}

	// set udp socket to non-blocking
	u_long mode = 1;
	ioctlsocket(udp_socket, FIONBIO, &mode);

	std::thread recvthread(udpListener);

	auto quitServer = []() {
		// quit server if `q` is received

		std::string ln;

		while (ln != "q")
			std::getline(std::cin, ln);
		};
	quitServer();

	freeaddrinfo(info);
	{
		std::lock_guard<std::mutex> udpInfoLock(udp_info_mutex);
		freeaddrinfo(udp_info);
	}

	// -------------------------------------------------------------------------
	// Clean-up after Winsock.
	//
	// WSACleanup()
	// -------------------------------------------------------------------------
	// free dynamically allocated memory
	udpListenerRunning = false;

	recvthread.join();

	WSACleanup();
}