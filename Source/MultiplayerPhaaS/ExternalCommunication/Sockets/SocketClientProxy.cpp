// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "SocketClientProxy.h"
#include "MultiplayerPhaaS/MultiplayerPhaaSLogging.h"

/** 
* The sockets connection map. The key is the server's id and the value the
* socket connection itself.
*/
TMap<int32, SOCKET> FSocketClientProxy::SocketConnectionsMap =
    TMap<int32, SOCKET>();

bool FSocketClientProxy::OpenSocketConnectionToServer
    (const FString& ServerIpAddr, const FString& ServerPort, 
     const int32 ServerId)
{
    MPHAAS_LOG_INFO
        (TEXT("Connecting to socket server \"%s:%s\""), *ServerIpAddr, 
         *ServerPort);

    // Initialize Winsock. This is needed to ensure the startup of the
    // environment to be able to use windows sockets
    if (!StartupWinsock())
    {
        // If startup fails, return false to avoid further processing
        return false;
    }

    // Get this client (local) addrinfo
    // This will get the server addrinfo
    addrinfo* AddrInfoResult = GetServerAddrInfo(ServerIpAddr, ServerPort);
    if (!AddrInfoResult)
    {
        return false;
    }

    // Attempt to connect to the socket server
    ConnectToServer(AddrInfoResult, ServerId);

    // Free memory as we don't need the addrinfo anymore
    freeaddrinfo(AddrInfoResult);

    // Check if we have found a valid connection with the request server
    if (GetSocketConnectionByServerId(ServerId) == INVALID_SOCKET)
    {
        // If not, clear winsock and return false to avoid further processing
        MPHAAS_LOG_ERROR
            (TEXT("Unable to connect to server! Must likely no server was found to connect to."));

        WSACleanup();
        return false;
    }

    // All good, ConnectionSocket is up and valid 
    MPHAAS_LOG_INFO(TEXT("Connection success."));
    return true;
}

bool FSocketClientProxy::StartupWinsock()
{
    // Initialize Winsock
    WSADATA WsaData;
    const int StartupResult = WSAStartup(MAKEWORD(2, 2), &WsaData);
    if (StartupResult != 0)
    {
        MPHAAS_LOG_ERROR(TEXT("WSAStartup failed with error: %d"),
            StartupResult);
        return false;
    }

    return true;
}

addrinfo* FSocketClientProxy::GetServerAddrInfo
    (const FString& ServerIpAddr, const FString& ServerPort)
{
    // Create Hints to pass as the addr information
    struct addrinfo Hints;
    ZeroMemory(&Hints, sizeof(Hints));

    // Set hits to get a socket of TCP protocol type
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    addrinfo* AddrResultInfo = NULL;
    const char* ServerIpAddrAsChar = TCHAR_TO_ANSI(*ServerIpAddr);
    const char* ServerPortAsChar = TCHAR_TO_ANSI(*ServerPort);
    const int AddrinfoReturnValue = getaddrinfo(ServerIpAddrAsChar, 
        ServerPortAsChar, &Hints, &AddrResultInfo);

    // Check for errors
    if (AddrinfoReturnValue != 0)
    {
        MPHAAS_LOG_ERROR(TEXT("GetClientAddrInfo() failed with error: %d"),
            AddrinfoReturnValue);

        // Clean up winsock
        WSACleanup();
        return NULL;
    }

    return AddrResultInfo;
}

void FSocketClientProxy::ConnectToServer(addrinfo* addrinfoToConnect, 
    const int32 ServerId)
{
    // The new connection socket
    SOCKET NewConnectionSocket = INVALID_SOCKET;

    // Attempt to connect to an address until one succeeds
    struct addrinfo* Ptr = NULL;
    for (Ptr = addrinfoToConnect; Ptr != NULL; Ptr = Ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        NewConnectionSocket = socket(Ptr->ai_family, Ptr->ai_socktype,
            Ptr->ai_protocol);
        if (NewConnectionSocket == INVALID_SOCKET)
        {
            MPHAAS_LOG_ERROR(TEXT("socket failed with error: %d\n"),
                WSAGetLastError());
            WSACleanup();
            return;
        }

        // Connect to server.
        const int ConnectResultValue = connect(NewConnectionSocket, 
            Ptr->ai_addr, (int)Ptr->ai_addrlen);

        // Check for errors. If error, just continue to try and get the
        // next connection
        if (ConnectResultValue == SOCKET_ERROR)
        {
            closesocket(NewConnectionSocket);
            NewConnectionSocket = INVALID_SOCKET;
            continue;
        }

        // If have found a valid connection, break and terminate 
        // processing as we've found a valid connection
        break;
    }

    // If successfully connected to server, add it to the socket connections 
    // map
    if (NewConnectionSocket != INVALID_SOCKET)
    {
        SocketConnectionsMap.Add(ServerId, NewConnectionSocket);
    }
}

bool FSocketClientProxy::CloseSocketConnectionsToServerById
    (const int32 TargetServerId)
{
    // Find the socket connection with its ID
    const auto TargetSocketConnection = 
        SocketConnectionsMap.Find(TargetServerId);
    
    if (!TargetSocketConnection || *TargetSocketConnection == INVALID_SOCKET)
    {
        MPHAAS_LOG_WARNING
            (TEXT("Socket connection with ID(%d) does not exist to be closed."),
            TargetServerId);
        return true;
    }

    // Shutdown the connection since no more data will be sent
    const int ShutdownResult = shutdown(*TargetSocketConnection, SD_SEND);

    // Check for shutdown errors
    if (ShutdownResult == SOCKET_ERROR)
    {
        MPHAAS_LOG_ERROR(TEXT("Shutdown failed with error: %d"),
            WSAGetLastError());
    }

    // Close socket
    closesocket(*TargetSocketConnection);

    // Cleanup winsock
    WSACleanup();

    // Remove server from the connections map
    SocketConnectionsMap.Remove(TargetServerId);

    return true;
}

FString FSocketClientProxy::SendMessageAndGetResponse(const char* Message,
    const int32 ServerId)
{
    MPHAAS_LOG_INFO
        (TEXT("Sending message to server with id: %d."), ServerId);

    // Get the server's socket connection
    auto TargetServerSocketConnection =
        GetSocketConnectionByServerId(ServerId);

    // Check if it's valid
    if (TargetServerSocketConnection == INVALID_SOCKET)
    {
        MPHAAS_LOG_ERROR
            (TEXT("Could not send message as socket connection is not valid."));
        return FString();
    }

    // Sending message
    const int SendReturn = send(TargetServerSocketConnection, Message,
        (int)strlen(Message), 0);

    // Check for error
    if (SendReturn == SOCKET_ERROR)
    {
        MPHAAS_LOG_ERROR(TEXT("Send failed with error: %d"),
            WSAGetLastError());

        // Cleanup winsock
        closesocket(TargetServerSocketConnection);
        WSACleanup();

        // Remove server from map as there is some error with it
        SocketConnectionsMap.Remove(ServerId);

        return FString();
    }

    // Setup buffer length to receive message. 
    // Any more characters will be discarded
    // TODO do a better solution for this
    char Recvbuf[DEFAULT_BUFLEN];
    const int Recvbuflen = DEFAULT_BUFLEN;

    // Final received message
    FString FinalReceivedMessage = FString();

    while (true)
    {
        MPHAAS_LOG_INFO(TEXT("Awaiting server response..."));

        // Await response from socket 
        // (this will stall the game thread until we receive a response)
        const int ReceiveReturn = recv(TargetServerSocketConnection, Recvbuf, 
            Recvbuflen, 0);

        // Check for errors
        if (ReceiveReturn < 0)
        {
            MPHAAS_LOG_ERROR(TEXT("Recv failed with error: %d"), 
                WSAGetLastError());
            return FString();
        }

        // Debug amount of bytes received
        MPHAAS_LOG_INFO(TEXT("Bytes received: %d"), ReceiveReturn);

        // Decode message received from socket as FString
        // The buffer will contain the message returned as char
        // The return value is the amount of bytes, and thus, the message 
        // length
        FString ReceivedMsgAsString(ReceiveReturn, Recvbuf);

        // Add receiving response to final message
        FinalReceivedMessage += ReceivedMsgAsString;

        // If message contains "MessageEnd", this is the final end message, 
        // return
        // If not, await for next recv as the rest of the message is still
        // comming
        if (FinalReceivedMessage.Contains("MessageEnd"))
        {
            return FinalReceivedMessage;
        }
    }

    return FinalReceivedMessage;
}

SOCKET FSocketClientProxy::GetSocketConnectionByServerId
    (const int32 TargetServerId)
{
    if (!SocketConnectionsMap.Contains(TargetServerId))
    {
        MPHAAS_LOG_ERROR
            (TEXT("ServerID \"%d\" does not exists on connection map."),
             TargetServerId);
        return INVALID_SOCKET;
    }

    return SocketConnectionsMap[TargetServerId];
}
