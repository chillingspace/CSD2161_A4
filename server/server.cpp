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

#include "server.h"
#include "game.h"

#define VERBOSE_LOGGING
#define JS_DEBUG
#define LOCALHOST_DEV

Server& Server::getInstance() {
	static Server instance;
	return instance;
}

/**
 * send data with udp.
 *
 * \param buffer
 * \param dest_addr
 * \return
 */
//int Server::sendData(const std::vector<char>& buffer, SESSION_ID sid) {
//	sockaddr_in* udp_addr_in = nullptr;
//	{
//		std::lock_guard<std::mutex> usersLock{ udp_clients_mutex };
//
//		if (udp_clients.find(sid) == udp_clients.end()) {
//			//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };		// potential deadlock
//			//std::cerr << "Session ID not found" << std::endl;
//			return SOCKET_ERROR;
//		}
//
//		udp_addr_in = reinterpret_cast<sockaddr_in*>(&udp_clients.at(sid));
//	}
//
//	if (udp_addr_in == nullptr) {
//		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
//		std::cerr << "udp_addr_in is nullptr" << std::endl;
//		return SOCKET_ERROR;
//	}
//
//	if (udp_socket == INVALID_SOCKET) {
//		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
//		std::cerr << "Invalid socket." << std::endl;
//		return SOCKET_ERROR;
//	}
//
//	int bytesSent = sendto(udp_socket, buffer.data(), (int)buffer.size(), 0, reinterpret_cast<sockaddr*>(&const_cast<sockaddr_in&>(*udp_addr_in)), sizeof(sockaddr_in));
//	if (bytesSent == SOCKET_ERROR) {
//		//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
//		char errorBuffer[256];
//		strerror_s(errorBuffer, sizeof(errorBuffer), errno);
//		//std::cerr << "sendto() failed: " << errorBuffer << std::endl;
//
//		int wsaError = WSAGetLastError();
//		//std::cerr << "sendto() failed with wsa error: " << wsaError << std::endl;
//	}
//	return bytesSent;
//}

int Server::sendData(const std::vector<char>& buffer, sockaddr_in udp_addr_in) {
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

int Server::broadcastData(const std::vector<char>& buffer) {
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

#ifndef LOCALHOST_DEV
	udp_addr_in.sin_addr.s_addr = htonl(INADDR_BROADCAST);  // Broadcast address 255.255.255.255, !TODO: test on multiple devices
#else
	const char* ip = "127.0.0.1";
	if (inet_pton(AF_INET, ip, &udp_addr_in.sin_addr) <= 0) {
		std::cerr << "Invalid address/Address not supported\n";
		return SOCKET_ERROR;
	}
#endif

	int bytesSent = sendto(udp_socket_broadcast, buffer.data(), (int)buffer.size(), 0, reinterpret_cast<sockaddr*>(&udp_addr_in), sizeof(sockaddr_in));
	if (bytesSent == SOCKET_ERROR) {
		//std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		char errorBuffer[256];
		strerror_s(errorBuffer, sizeof(errorBuffer), errno);
		std::cerr << "sendto() failed: " << errorBuffer << std::endl;

		int wsaError = WSAGetLastError();
		std::cerr << "sendto() failed with wsa error: " << wsaError << std::endl;
	}
	{
		/*std::lock_guard<std::mutex> soutlock(_stdoutMutex);
		std::cout << "Sending packet: ";
		for (int i = 0; i < buffer.size(); ++i) {
			std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(uint8_t)buffer[i] << " ";
		}
		std::cout << std::dec << std::endl;*/
	}
	return bytesSent;
}

int Server::getSessionId() {
	return session_id++;
}

SOCKET Server::createUdpSocket(int port, bool broadcast) {
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
    if (errorCode == SOCKET_ERROR) {
        std::cerr << "bind() failed." << std::endl;
        closesocket(udpSocket);
        freeaddrinfo(udp_info);
        WSACleanup();
        return INVALID_SOCKET;
    }

	// get assigned port number
	sockaddr_in udpAddr{};
	socklen_t addrLen = sizeof(udpAddr);
	int result_port;
	if (getsockname(udpSocket, (sockaddr*)&udpAddr, &addrLen) == 0) {
		result_port = ntohs(udpAddr.sin_port); // Convert from network byte order
		std::cout << "Assigned port: " << result_port << std::endl;
	}
	else {
		std::cerr << "getsockname() failed." << std::endl;
		closesocket(udpSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Cleanup
	return udpSocket;
}

/**
 * worker thread. should only be used in 1 thread in any given time.
 *
 */
void Server::udpListener() {
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
			{
				std::lock_guard<std::mutex> acklock(ack_conn_request_clients_mutex);
				ack_conn_request_clients.insert(sid);
			}
			{
				std::lock_guard<std::mutex> stdoutlock(_stdoutMutex);
				std::cout << "Client with SID " << sid << " acknowledged connection request." << std::endl;
			}
			break;
		}
		case ACK_START_GAME: {
			isAck = true;
			{
				std::lock_guard<std::mutex> acklock(ack_start_game_clients_mutex);
				ack_start_game_clients.insert(sid);
			}
			{
				std::lock_guard<std::mutex> stdoutlock(_stdoutMutex);
				std::cout << "Client with SID " << sid << " acknowledged start game." << std::endl;
				std::cout << "Current ack count: " << ack_start_game_clients.size() << std::endl;

			}
			break;
		}
		//case ACK_ALL_ENTITIES: {
		//	isAck = true;
		//	std::lock_guard<std::mutex> acklock(ack_all_entities_clients_mutex);
		//	ack_all_entities_clients.insert(sid);
		//	break;
		//}
		case ACK_END_GAME: {
			isAck = true;
			{
				std::lock_guard<std::mutex> acklock(ack_end_game_clients_mutex);
				ack_end_game_clients.insert(sid);
			}
			{
				std::lock_guard<std::mutex> stdoutlock(_stdoutMutex);
				std::cout << "Client with SID " << sid << " acknowledged end game." << std::endl;
			}
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


void Server::requestHandler() {

	while (udpListenerRunning) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		// get data
		std::deque<std::pair<sockaddr_in, std::vector<char>>> recvbuffer;
		{
			std::lock_guard<std::mutex> lock(recvbuffer_queue_mutex);

			recvbuffer = recvbuffer_queue;
			recvbuffer_queue.clear();
		}

		//static std::vector<char> sbuf(MAX_PACKET_SIZE);
		//if (sbuf.size() != MAX_PACKET_SIZE)
		//	sbuf.resize(MAX_PACKET_SIZE);
		static std::vector<char> sbuf(16);

		// handle data
		for (const auto [senderAddr, rbuf] : recvbuffer) {
			const int cmd = rbuf[0];

			switch (cmd) {
			case CONN_REQUEST: {
				std::cout << "Connection Requested By Client" << std::endl;
				int num_players{};
				{
					std::lock_guard<std::mutex> spaceshipsdatalock(Game::getInstance().data_mutex);
					num_players = (int)Game::getInstance().data.spaceships.size();
				}

				if (num_players >= Game::MAX_PLAYERS || Game::getInstance().gameRunning) {
					sbuf[0] = CONN_REJECTED;
					sendData(sbuf, senderAddr);
					break;
				}

				// player allowed
				const int sid = getSessionId();

				// create new player spaceship
				Game::Spaceship new_spaceship{};
				new_spaceship.pos = { 0, 0 };
				new_spaceship.vector = { 0, 0 };
				new_spaceship.radius = Game::SPACESHIP_RADIUS;
				new_spaceship.sid = sid;
				new_spaceship.rotation = 0.f;
				new_spaceship.lives_left = Game::NUM_START_LIVES;
				new_spaceship.score = 0;
				{
					std::lock_guard<std::mutex> spaceshipsdatalock(Game::getInstance().data_mutex);
					Game::getInstance().data.spaceships.push_back(new_spaceship);
				}

				int buf_idx{};

				sbuf[buf_idx++] = CONN_ACCEPTED;

				// session id
				sbuf[buf_idx++] = sid;

				// broadcast port
				sbuf[buf_idx++] = (serverUdpPortBroadcast >> 8) & 0xff;
				sbuf[buf_idx++] = serverUdpPortBroadcast & 0xff;

				// spawn locations (world pos)
				constexpr float spawnX = 500.f;
				constexpr float spawnY = 500.f;

				memcpy(sbuf.data() + buf_idx, t_to_bytes(spawnX).data(), sizeof(float));
				buf_idx += (int)sizeof(float);
				memcpy(sbuf.data() + buf_idx, t_to_bytes(spawnY).data(), sizeof(float));
				buf_idx += (int)sizeof(float);

				// spawn rotation
				constexpr float rotation_deg = 15.1f;

				memcpy(sbuf.data() + buf_idx, t_to_bytes(rotation_deg).data(), sizeof(float));
				buf_idx += (int)sizeof(float);

				//// num lives
				//sbuf[buf_idx++] = Game::NUM_START_LIVES;

				auto reliableSender = [&]() {
					std::chrono::duration<float> elapsed{};
					auto start = std::chrono::high_resolution_clock::now();
					char senderIP[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(senderAddr.sin_addr), senderIP, INET_ADDRSTRLEN);
					std::cout << "Received data from " << senderIP << ":" << ntohs(senderAddr.sin_port) << std::endl;

					while (true) {
#ifdef VERBOSE_LOGGING
						{
							// Debug: Print packet contents before sending
							std::stringstream ss;
							ss << "Sending packet: ";
							for (int i = 0; i < sbuf.size(); ++i) {
								ss << std::hex << std::setw(2) << std::setfill('0') << (int)(uint8_t)sbuf[i] << " ";
							}
							ss << std::dec << "\n";

							// Send data
							int bytesSent = sendData(sbuf, senderAddr);
							if (bytesSent < 0) {
								ss << "sendData() failed: " << "\n";
							}
							else {
								ss << "Sent " << bytesSent << " bytes to " << senderIP << ":" << ntohs(senderAddr.sin_port) << "\n";
							}

							{
								std::lock_guard<std::mutex> stdoutlock(_stdoutMutex);
								std::cout << ss.str() << std::endl;
							}
						}
#endif


						{
							std::lock_guard<std::mutex> conn_req_lock(ack_conn_request_clients_mutex);
							if (ack_conn_request_clients.find(sid) != ack_conn_request_clients.end()) {
								// client has acked.
								ack_conn_request_clients.erase(sid);
								break;
							}
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));

						elapsed = std::chrono::high_resolution_clock::now() - start;
						if (elapsed.count() >= DISCONNECTION_TIMEOUT_DURATION_MS / 1000.f) {
							// disconnect client
							{
								std::lock_guard<std::mutex> spaceshipsdatalock(Game::getInstance().data_mutex);
								auto it = std::find_if(Game::getInstance().data.spaceships.begin(), Game::getInstance().data.spaceships.end(), [sid](const Game::Spaceship& s) { return s.sid == sid; });
								Game::getInstance().data.spaceships.erase(it);
							}
							{
								std::lock_guard<std::mutex> stdoutlock(_stdoutMutex);
								std::cout << "Client timed out(disconnected): " << sid << std::endl;
							}
							return;
						}
					}
					{
						std::lock_guard<std::mutex> stdoutlock(_stdoutMutex);
						std::cout << "Connection accepted. Sent data to client. Client ACKed SID: " << sid << std::endl;
					}
					};
				//std::thread t(reliableSender);
				{
					std::lock_guard<std::mutex> tplock(threadpool_mutex);
					threadpool.push_back(std::async(std::launch::async, reliableSender));
				}
				break;
			}
			case REQ_START_GAME: {
				std::cout << "Received start game request." << std::endl;

				if (Game::getInstance().gameRunning) {
					return;
				}

				std::vector<char> buf(16);
				//buf.resize(MAX_PACKET_SIZE);

				buf[0] = START_GAME;

				int num_conns;
				{
					std::lock_guard<std::mutex> lock(Game::getInstance().data_mutex);
					num_conns = (int)Game::getInstance().data.spaceships.size();
				}

				auto bc = [this, &buf, num_conns]() {  

					int num_acks = 0;

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
								std::lock_guard<std::mutex> clientslock(Game::getInstance().data_mutex);

								// remove sessions that timed out
								for (auto it = Game::getInstance().data.spaceships.begin(); it != Game::getInstance().data.spaceships.end();) {
									if (acked_clients.find(it->sid) != acked_clients.end()) {
										++it;
										continue;
									}
									it = Game::getInstance().data.spaceships.erase(it);
								}
								// num_conns = (int)Game::getInstance().data.spaceships.size();
							}

							break;
						}


						// Check the number of acks
						{
							std::lock_guard<std::mutex> acklock(ack_start_game_clients_mutex);
							num_acks = (int)ack_start_game_clients.size();
						}

						std::cout << "Current num_acks: " << num_acks << " / num_conns: " << num_conns << std::endl;
						if (num_acks >= num_conns) {
							std::cout << "Starting Game" << std::endl;
							Game::getInstance().data.reset();
							Game::getInstance().gameRunning = true;
							break;
						}

						// Continue broadcasting to request acks
						broadcastData(buf);
						std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));

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
				// locking for this entire block to prevent overwriting from gameUpdate
				std::lock_guard<std::mutex> lock(Game::getInstance().data_mutex);

				const SESSION_ID sid = rbuf[1];

				int idx = 2;

				auto spaceship = std::find_if(
					Game::getInstance().data.spaceships.begin(),
					Game::getInstance().data.spaceships.end(),
					[&sid](const Game::Spaceship& s) { return s.sid == sid; }
				);

				// pos x
				std::vector<char> bytes(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
				spaceship->pos.x = btof(bytes);
				idx += (int)sizeof(float);

				// pos y
				bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
				spaceship->pos.y = btof(bytes);
				idx += (int)sizeof(float);

				// vector x
				bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
				spaceship->vector.x = btof(bytes);
				idx += (int)sizeof(float);

				// vector y
				bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
				spaceship->vector.y = btof(bytes);
				idx += (int)sizeof(float);

				// rotation
				bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
				spaceship->rotation = btof(bytes);
				idx += (int)sizeof(float);

				// num new bullets fired
				const int new_bullets = rbuf[idx++];

				for (int i{}; i < new_bullets; i++) {
					Game::Bullet b;

					b.sid = sid;

					// pos x
					bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
					b.pos.x = btof(bytes);
					idx += (int)sizeof(float);

					// pos y
					bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
					b.pos.y = btof(bytes);
					idx += (int)sizeof(float);

					// vector x
					bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
					b.vector.x = btof(bytes);
					idx += (int)sizeof(float);

					// vector y
					bytes.assign(rbuf.begin() + idx, rbuf.begin() + sizeof(float));
					b.vector.y = btof(bytes);
					idx += (int)sizeof(float);

					Game::getInstance().data.bullets.push_back(b);
				}

			}
			}
		}
	}
}


int Server::init() {
	std::string udpPortString;

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

	server_info = nullptr;
	errorCode = getaddrinfo(hostname, udpPortString.c_str(), &hints, &server_info);
	if ((errorCode) || (server_info == nullptr))
	{
		std::cerr << "getaddrinfo() failed." << std::endl;
		WSACleanup();
		return errorCode;
	}

	/* PRINT SERVER IP ADDRESS AND PORT NUMBER */
	struct sockaddr_in* serverAddress = reinterpret_cast<struct sockaddr_in*> (server_info->ai_addr);
	inet_ntop(AF_INET, &(serverAddress->sin_addr), serverIPAddr, INET_ADDRSTRLEN);
	getnameinfo(server_info->ai_addr, static_cast <socklen_t> (server_info->ai_addrlen), serverIPAddr, sizeof(serverIPAddr), nullptr, 0, NI_NUMERICHOST);
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
	if (udp_socket_broadcast == INVALID_SOCKET) {
		closesocket(udp_socket_broadcast);
		udp_socket_broadcast = INVALID_SOCKET;
		WSACleanup();
		std::lock_guard<std::mutex> usersLock{ _stdoutMutex };
		std::cerr << "Failed to create UDP Broadcast socket" << std::endl;
		return 5;
	}

	// set udp socket to non-blocking
	u_long mode = 1;
	ioctlsocket(udp_socket, FIONBIO, &mode);

	return 0;
}


void Server::cleanup() {
	freeaddrinfo(server_info);
	{
		std::lock_guard<std::mutex> udpInfoLock(udp_info_mutex);
		freeaddrinfo(udp_info);
	}
}


/* thread management */

std::deque<std::future<void>> Server::threadpool;
std::mutex Server::threadpool_mutex;

void Server::threadpoolManager() {
	while (Server::getInstance().udpListenerRunning) {
		std::vector<std::future<void>> pool;
		{
			std::lock_guard<std::mutex> lock(threadpool_mutex);
			pool.insert(pool.end(), std::make_move_iterator(threadpool.begin()), std::make_move_iterator(threadpool.end()));
			threadpool.clear();
		}


		for (auto& fut : pool) {
			fut.get();	// waits till thread has completed
		}

		// at this point, all threads have completed
	}
}
