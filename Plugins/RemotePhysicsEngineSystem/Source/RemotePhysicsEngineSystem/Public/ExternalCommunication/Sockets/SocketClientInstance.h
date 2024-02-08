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

#include "SocketClientInstance.generated.h"

/**
 * 
 */
UCLASS()
class REMOTEPHYSICSENGINESYSTEM_API USocketClientInstance : public UObject
{
	GENERATED_BODY()

public:
	/**
	* Opens connection with the a phyiscs service server. Returns a boolean
	* informing if the connection was succesful.
	*
	* @param ServerIpAddr The server's ip address to connect to
	* @param ServerPort The server's port to connect to
	* @param ServerId The server's id. Used to easily find the created physics
	* service server on the SocketsConnectionMap
	*
	* @return True if the connection was opened succesfully. False otherwise.
	*/
	bool OpenSocketConnectionToServer(const FString& ServerIpAddr,
		const FString& ServerPort);

	/** */
	bool CloseSocketConnection();

	/**
	* Sends a message to the physics service server and awaits a response.
	* This should be used to send physics requests to the physics service, such
	* as initialization and update.
	*
	* @param Message The message to forward to the socket server with the given
	* message
	*
	* @return The physics service's server response to such message
	*/
	FString SendMessageAndGetResponse(const char* Message);

public:
	/**
	* Check if a connection is valid with a given physics service id.
	*
	* @return True if the connection is valid. False otherwise
	*/
	inline bool IsConnectionValid() const
		{ return SocketConnection != INVALID_SOCKET; }

private:
	/** Starts up the winsock library. */
	bool StartupWinsock();

	/**
	* Get the addrinfo from the server as a TCP socket.
	*
	* @param ServerIpAddr The server's ip address to connect to
	* @param ServerPort The server's port to connect to
	*
	* @return The server's addrinfo
	*/
	addrinfo* GetServerAddrInfo(const FString& ServerIpAddr,
		const FString& ServerPort);

	/**
	* Connects to a given addrinfo. This should be the TCP socket addrinfo to
	* connect to the physics service server.
	*
	* @param AddrinfoToConnect The physics service server addrinfo
	* @param ServerId The server's id. Used to easily find the created physics
	* service server on the SocketConnectionsMap
	*/
	void ConnectToServer(addrinfo* AddrinfoToConnect);

private:
	/**
	* The sockets connection map. The key is the server's id and the value the
	* socket connection itself.
	*/
	SOCKET SocketConnection = INVALID_SOCKET;
};
