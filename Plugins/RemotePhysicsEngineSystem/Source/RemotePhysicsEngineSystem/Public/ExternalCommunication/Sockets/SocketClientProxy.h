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
* This class deals with the socket communication with the physics services
* that acts as a server, opening a socket on a given server ip (with port)
* and attributing it a given id to easily find it afterwards
* 
* The pipeline is:
* [Game]SocketClientProxy -> [PhysicsService]SocketServerProxy and
* [PhysicsService]SocketServerProxy -> [Game]SocketClientProxy
* 
* - The game works as a socket client.
* - The physics services works as socket servers
* 
* Therefore, this class deals with such communication, opening/closing
* socket connections, storing the current set of physics services, and etc. 
* Also, it forwards the messages to the socket servers to 
* request its initialization and physics simulation updates.
*/
class REMOTEPHYSICSENGINESYSTEM_API FSocketClientProxy
{
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
	static bool OpenSocketConnectionToServer
		(const FString& ServerIpAddr, const FString& ServerPort,
		const int32 ServerId);

	/** 
	* Closes the socket connection with a server by its id.
	* 
	* @param TargetServerId The server's id to close connection
	* 
	* @return True if the closing was successful. False otherwise
	*/
	static bool CloseSocketConnectionsToServerById(const int32 TargetServerId);

	/** 
	* Sends a message to the physics service server and awaits a response.
	* This should be used to send physics requests to the physics service, such
	* as initialization and update. 
	* 
	* @param Message The message to forward to the socket server with the given
	* message
	* @param ServerId The server id to send the message to
	* 
	* @return The physics service's server response to such message
	*/
	static FString SendMessageAndGetResponse(const char* Message,
		const int32 ServerId);
	
public:
	/** 
	* Check if a connection is valid with a given physics service id. 
	* 
	* @return True if the connection is valid. False otherwise
	*/
	inline static bool IsConnectionValid(const int32 PhysicsServiceId)
	{ 
		// Get the target socket connection given its ID on the map
		const auto TargetSocketConnection = 
			SocketConnectionsMap.Find(PhysicsServiceId);

		// If invalid, return false
		if (TargetSocketConnection == nullptr ||
			*TargetSocketConnection == INVALID_SOCKET)
		{
			return false;
		}

		return true; 
	}

	/** Getter to the current number of active physics services */
	inline static int32 GetNumberOfPhysicsServices()
		{ return SocketConnectionsMap.Num(); }

private:
	/** Starts up the winsock library. */
	static bool StartupWinsock();

	/** 
	* Get the addrinfo from the server as a TCP socket. 
	* 
	* @param ServerIpAddr The server's ip address to connect to
	* @param ServerPort The server's port to connect to
	* 
	* @return The server's addrinfo
	*/
	static addrinfo* GetServerAddrInfo(const FString& ServerIpAddr,
		const FString& ServerPort);

	/** 
	* Get the socket connection by the server's id.
	* 
	* @param TargetServerId The server id to get the socket connection
	* 
	* @return The socket connection given the server's id
	* 
	* @note The SOCKET connection may be invalid if this server's id does not
	* exists on the connection map
	*/
	static SOCKET GetSocketConnectionByServerId(const int32 TargetServerId);

	/** 
	* Connects to a given addrinfo. This should be the TCP socket addrinfo to 
	* connect to the physics service server.
	* 
	* @param AddrinfoToConnect The physics service server addrinfo
	* @param ServerId The server's id. Used to easily find the created physics
	* service server on the SocketConnectionsMap
	*/
	static void ConnectToServer(addrinfo* AddrinfoToConnect,
		const int32 ServerId);

private:
	/**
	* The sockets connection map. The key is the server's id and the value the
	* socket connection itself.
	*/
	static TMap<int32, SOCKET> SocketConnectionsMap;
};
