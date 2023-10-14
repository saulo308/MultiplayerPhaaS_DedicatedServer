// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

/** The default buffer lenght for the socket connection. */
#define DEFAULT_BUFLEN 1048576

/** 
* The default port to open the socket connection. This should be the same
* set on the SocketServerProxy
*/
#define DEFAULT_PORT "27015"

/**
* This class deals with the socket communication with a the physics service 
* that acts as a server, opening a socket on a given port (DEFAULT_PORT).
* 
* The pipeline is:
* [Game]SocketClientProxy -> [PhysicsService]SocketServerProxy and
* [PhysicsService]SocketServerProxy -> [Game]SocketClientProxy
* 
* - The game works as a socket client.
* - The physics service works as a socket server
* 
* Therefore, this class deals with such communication, opening/closing the
* socket connection. Also, it forwards the messages to the socket server to 
* request its initialization and physics simulation update to the PSDActors.
*/
class MULTIPLAYERPHAAS_API FSocketClientProxy
{
public:
	/** 
	* Opens connection with the localhost socket server. Returns a boolean
	* informing if the connection was succesful.
	* 
	* @return True if the connection was opened succesfully. False otherwise.
	*/
	static bool OpenSocketConnectionToLocalhostServer
		(const FString& SocketServerIpAddr);

	/** Closes the opened socket connection to the localhost server. */
	static bool CloseSocketConnection();

	/** 
	* Sends a message to the localhost socket server and awaits a response.
	* This should be used to send physics requests to the physics service, such
	* as initialization and update. 
	* 
	* @param Message The message to forward to the socket server that will
	* send a gRPC to the physics service.
	* 
	* @return The physics service's response to such message
	*/
	static FString SendMessageAndGetResponse(const char* Message);
	
public:
	/** Getter to know if the socket connection is valid. */
	inline static bool HasValidConnection() 
		{ return ConnectionSocket != INVALID_SOCKET; }

private:
	/** Starts up the winsock library. */
	static bool StartupWinsock();

	/** 
	* Get the addrinfo to the localhost socket server as a TCP socket. 
	*/
	static addrinfo* GetServerAddrInfo(const FString& SocketServerIpAddr);

	/** 
	* Connects to a given addrinfo. This should be the localhost TCP socket
	* addrinfo to connect to the localhost socket server.
	* 
	* @param addrinfoToConnect The localhost's socket server addrinfo
	*/
	static void ConnectSocketToLocalSocketServer(addrinfo* addrinfoToConnect);

private:
	/** The socket to connect to the physics service server. */
	static SOCKET ConnectionSocket;
};
