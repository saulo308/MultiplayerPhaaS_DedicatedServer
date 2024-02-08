// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RemotePhysicsEngineSystem/Public/ExternalCommunication/Sockets/SocketClientInstance.h"

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
	* Get the socket connection by the server's id.
	*
	* @param TargetServerId The server id to get the socket connection
	*
	* @return The socket connection given the server's id
	*
	* @note The SOCKET connection may be invalid if this server's id does not
	* exists on the connection map
	*/
	static USocketClientInstance* GetSocketConnectionByServerId
		(const int32 TargetServerId);

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

		// Return true if target connection exists and its connection is valid
		return TargetSocketConnection != nullptr && 
			(*TargetSocketConnection)->IsConnectionValid();
	}

	/** Getter to the current number of active physics services */
	inline static int32 GetNumberOfPhysicsServices()
		{ return SocketConnectionsMap.Num(); }

private:
	/**
	* The sockets connection map. The key is the server's id and the value the
	* socket connection itself.
	*/
	static TMap<int32, USocketClientInstance*> SocketConnectionsMap;
};
