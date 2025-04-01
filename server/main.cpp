#include "server.h"
#include "game.h"


#define VERBOSE_LOGGING
#define JS_DEBUG

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

	auto quitServerListener = []() {
		// quit server if `q` is received

		std::string ln;

		while (ln != "q")
			std::getline(std::cin, ln);
		};
	quitServerListener();

	s.cleanup();

	// ----------~---------------------------------------------------------------
	// Clean-up after Winsock.
	//
	// WSACleanup()
	// -------------------------------------------------------------------------
	// free dynamically allocated memory
	s.udpListenerRunning = false;
	Game::getInstance().gameRunning = false;
	Game::getInstance().gameThreadRunning = false;

	recvthread.join();
	gameUpdateThread.join();
	reqHandlerThread.join();

	WSACleanup();

	return 0;
}
