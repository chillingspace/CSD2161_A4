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

#include "taskqueue.h"

using SESSION_ID = int;

constexpr int MAX_PACKET_SIZE = 1000;

char serverIPAddr[MAX_PACKET_SIZE];
int serverUdpPort{};
addrinfo* udp_info = nullptr;
std::mutex udp_info_mutex;
SESSION_ID session_id{};
SOCKET udp_socket{};

std::unordered_map<SESSION_ID, sockaddr_in> udp_clients;
std::mutex udp_clients_mutex;

std::unordered_map<SESSION_ID, bool> client_acks;
std::mutex client_acks_mutex;

std::unordered_map<SESSION_ID, float> client_last_request_time;		// used to timeout client connection
std::mutex client_last_request_time_mutex;

constexpr int DISCONNECTION_TIMEOUT_DURATION_MS = 15000;
constexpr int TIMEOUT_MS = 200;		// timeout before retrying

bool udpListenerRunning = true;
int sendData(const std::vector<char>& buffer, SESSION_ID sid);

constexpr int MAX_PACKET_QUEUE = 100;
std::deque<std::vector<char>> recvbuffer_queue;
std::mutex recvbuffer_queue_mutex;


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

int getSessionId() {
	return session_id++;
}

SOCKET createUdpSocket(int port) {
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
	serverUdpPort = result_port;		// to check if port is assigned correctly

	//freeaddrinfo(udp_info);
	return udpSocket;
}

/**
 * uses stop and wait to ensure client acks before performing next action.
 * 
 * \param buffer
 * \param sid
 * \return 
 */
bool sendReliableUdp(const std::vector<char>& buffer, SESSION_ID sid) {
	bool ack = false;
	float elapsed_time{};

	auto start = std::chrono::high_resolution_clock::now();

	while (!ack) {
		auto curr = std::chrono::high_resolution_clock::now();
		if (curr - start > std::chrono::seconds(DISCONNECTION_TIMEOUT_DURATION_MS)) {
			// disconnect udp client if taking too long to respond
			std::lock_guard<std::mutex> lock(udp_clients_mutex);
			if (udp_clients.find(sid) != udp_clients.end()) {
				udp_clients.erase(sid);
			}
			return false;
		}


		int bytesSent = sendData(buffer, sid);

		{
			std::lock_guard<std::mutex> lock(client_acks_mutex);
			ack = client_acks[sid];
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
	}
	return true;
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

		{
			std::lock_guard<std::mutex> lock(recvbuffer_queue_mutex);
			recvbuffer_queue.push_back(recvbuffer);
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
		std::cout << "Server UDP Port Number: " << udpPortString << std::endl;
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