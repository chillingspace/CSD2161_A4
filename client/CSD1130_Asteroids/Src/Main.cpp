/******************************************************************************/
/*!
\file		Main.cpp
\author		GWEE BOON XUEN SEAN, g.boonxuensean, 2301326
\par		g.boonxuensean@digipen.edu
\date		08 Feb 2024
\brief		This file contains the game engine.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

//#include <winsock2.h>  // Include first
#include <ws2tcpip.h>  // Additional networking functions
#include <Windows.h>   // Include after winsock2.h

#include "main.h"
#include <memory>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
// ---------------------------------------------------------------------------
// Globals
float	 g_dt;
double	 g_appTime;

SOCKET clientSocket;
sockaddr_in serverAddr;


void InitClient()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		std::cerr << "Failed to create socket!\n";
		return;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(54000);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);


	std::cout << "Client initialized. Ready to send input to server.\n";
}

void SendInput(const std::string& input)
{
	sendto(clientSocket, input.c_str(), input.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
}

void ReceiveGameState()
{
	char buffer[1024];
	int serverSize = sizeof(serverAddr);
	int bytesReceived = recvfrom(clientSocket, buffer, 1024, 0, (sockaddr*)&serverAddr, &serverSize);

	if (bytesReceived > 0)
	{
		buffer[bytesReceived] = '\0';
		std::cout << "Game State: " << buffer << std::endl;
		// TODO: Apply received game state to game objects
	}
}

/******************************************************************************/
/*!
	Starting point of the application
*/
/******************************************************************************/
int WINAPI WinMain(HINSTANCE instanceH, HINSTANCE prevInstanceH, LPSTR command_line, int show)
{
	UNREFERENCED_PARAMETER(prevInstanceH);
	UNREFERENCED_PARAMETER(command_line);

	//// Enable run-time memory check for debug builds.
	//#if defined(DEBUG) | defined(_DEBUG)
	//	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	//#endif

	//int * pi = new int;
	////delete pi;


	// Initialize the system
	AESysInit (instanceH, show, 800, 600, 1, 60, false, NULL);

	// Changing the window title
	AESysSetWindowTitle("Asteroids Demo!");

	//set background color
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);


	InitClient(); // Initialize UDP client
	GameStateMgrInit(GS_ASTEROIDS);

	while(gGameStateCurr != GS_QUIT)
	{
		// reset the system modules
		AESysReset();

		// If not restarting, load the gamestate
		if(gGameStateCurr != GS_RESTART)
		{
			GameStateMgrUpdate();
			GameStateLoad();
		}
		else
			gGameStateNext = gGameStateCurr = gGameStatePrev;

		// Initialize the gamestate
		GameStateInit();

		while(gGameStateCurr == gGameStateNext)
		{
			AESysFrameStart();

			GameStateUpdate();

			GameStateDraw();
			
			AESysFrameEnd();

			// Send input to server (Example: Move command)
			SendInput("MOVE_RIGHT");
			ReceiveGameState(); // Receive latest game state

			// check if forcing the application to quit
			if ((AESysDoesWindowExist() == false) || AEInputCheckTriggered(AEVK_ESCAPE))
				gGameStateNext = GS_QUIT;

			g_dt = (f32)AEFrameRateControllerGetFrameTime();
			g_appTime += g_dt;
		}
		
		GameStateFree();

		if(gGameStateNext != GS_RESTART)
			GameStateUnload();

		gGameStatePrev = gGameStateCurr;
		gGameStateCurr = gGameStateNext;
	}

	// free the system
	closesocket(clientSocket);
	WSACleanup();
	AESysExit();
}