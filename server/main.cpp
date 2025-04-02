/* Start Header
*****************************************************************/
/*!
\file main.cpp
\author Poh Jing Seng, 2301363
\par jingseng.poh\@digipen.edu
\date 1 Apr 2025
\brief
This is the main entry point into the program
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

#include "server.h"
#include "game.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


int main()
{
	Server& s = Server::getInstance();
	int serverExitCode = s.init();

	std::thread recvthread([&s]() { s.udpListener(); });
	std::thread gameUpdateThread([&]() { 
		while (Game::getInstance().gameThreadRunning) {
			Game::getInstance().updateGame();
		}
		}
	);
	std::thread reqHandlerThread([&s]() {s.requestHandler(); });
	std::thread keepAliveCheckingThread([&s]() {s.keepAliveChecker(); });
	std::thread threadpoolManagerThread(Server::threadpoolManager);

	auto quitServerListener = []() {
		// quit server if `q` is received

		std::string ln;

		while (ln != "q")
			std::getline(std::cin, ln);
		};
	quitServerListener();

	s.udpListenerRunning = false;
	Game::getInstance().gameRunning = false;
	Game::getInstance().gameThreadRunning = false;

	recvthread.join();
	gameUpdateThread.join();
	reqHandlerThread.join();
	keepAliveCheckingThread.join();
	threadpoolManagerThread.join();

	s.cleanup();

	WSACleanup();

	return 0;
}
