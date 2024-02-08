// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

/** 
* The sockets connection map. The key is the server's id and the value the
* socket connection itself.
*/
TMap<int32, USocketClientInstance*> FSocketClientProxy::SocketConnectionsMap =
    TMap<int32, USocketClientInstance*>();

bool FSocketClientProxy::OpenSocketConnectionToServer
    (const FString& ServerIpAddr, const FString& ServerPort, 
     const int32 ServerId)
{
    RPES_LOG_INFO(TEXT("Connecting to socket server \"%s:%s\""), *ServerIpAddr, 
         *ServerPort);

    // Create a new socket client instance
    auto* NewSocketClientInstance = NewObject<USocketClientInstance>();

    // Open connection to server
    NewSocketClientInstance->OpenSocketConnectionToServer(ServerIpAddr, 
        ServerPort);

    // Check if we have found a valid connection with the request server
    if (!NewSocketClientInstance->IsConnectionValid())
    {
        // If not, just log it
        RPES_LOG_ERROR(TEXT("Unable to connect to server! Must likely no "
            "server was found to connect to."));
        return false;
    }

    // All good, ConnectionSocket is up and valid 
    RPES_LOG_INFO(TEXT("Connection to server \"%s:%s\" was successful."),
        *ServerIpAddr, *ServerPort);

    // Add the new connection to the socket connections map
    SocketConnectionsMap.Add(ServerId, NewSocketClientInstance);

    return true;
}

bool FSocketClientProxy::CloseSocketConnectionsToServerById
    (const int32 TargetServerId)
{
    // Find the socket connection with its ID
    const auto TargetSocketConnection = 
        SocketConnectionsMap.Find(TargetServerId);
    
    // Check if such socket connection is valid
    if (!TargetSocketConnection || 
        !(*TargetSocketConnection)->IsConnectionValid())
    {
        RPES_LOG_WARNING(TEXT("Socket connection with id (%d) does not exist \
            to be closed."), TargetServerId);
        return true;
    }

    // Close the socket connection with server
    (*TargetSocketConnection)->CloseSocketConnection();

    // Remove connection from the socket connections map
    SocketConnectionsMap.Remove(TargetServerId);

    return true;
}

USocketClientInstance* FSocketClientProxy::GetSocketConnectionByServerId
    (const int32 TargetServerId)
{
    // Check if such id is valid on socket connections map
    if (!SocketConnectionsMap.Contains(TargetServerId))
    {
        RPES_LOG_ERROR(TEXT("Server connection with id \"%d\" does not exist "
            "on socket connection map."), TargetServerId);
        return nullptr;
    }

    return SocketConnectionsMap[TargetServerId];
}
