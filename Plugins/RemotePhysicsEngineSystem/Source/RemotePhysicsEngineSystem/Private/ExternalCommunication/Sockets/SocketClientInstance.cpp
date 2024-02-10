// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ExternalCommunication/Sockets/SocketClientInstance.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"

bool USocketClientInstance::OpenSocketConnectionToServer
    (const FString& ServerIpAddr, const FString& ServerPort)
{
    RPES_LOG_INFO(TEXT("Connecting to socket server \"%s:%s\""), *ServerIpAddr,
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
    ConnectToServer(AddrInfoResult);

    // Free memory as we don't need the addrinfo anymore
    freeaddrinfo(AddrInfoResult);

    // Check if we have found a valid connection with the request server
    if (!IsConnectionValid())
    {
        // If not, clear winsock and return false to avoid further processing
        RPES_LOG_ERROR(TEXT("Unable to connect to server! Must likely no \
            server was found to connect to."));

        WSACleanup();

        return false;
    }

    // All good, ConnectionSocket is up and valid 
    RPES_LOG_INFO(TEXT("Connection success."));
    return true;
}

bool USocketClientInstance::StartupWinsock()
{
    // Initialize Winsock
    WSADATA WsaData;
    const int StartupResult = WSAStartup(MAKEWORD(2, 2), &WsaData);
    if (StartupResult != 0)
    {
        RPES_LOG_ERROR(TEXT("WSAStartup failed with error: %d"),
            StartupResult);
        return false;
    }

    return true;
}

addrinfo* USocketClientInstance::GetServerAddrInfo(const FString& ServerIpAddr, 
    const FString& ServerPort)
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
        RPES_LOG_ERROR(TEXT("GetClientAddrInfo() failed with error: %d"),
            AddrinfoReturnValue);

        // Clean up winsock
        WSACleanup();
        return NULL;
    }

    return AddrResultInfo;
}

void USocketClientInstance::ConnectToServer(addrinfo* addrinfoToConnect)
{
    // Attempt to connect to an address until one succeeds
    struct addrinfo* Ptr = NULL;
    for (Ptr = addrinfoToConnect; Ptr != NULL; Ptr = Ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        SocketConnection = socket(Ptr->ai_family, Ptr->ai_socktype,
            Ptr->ai_protocol);
        if (SocketConnection == INVALID_SOCKET)
        {
            RPES_LOG_ERROR(TEXT("socket failed with error: %d\n"),
                WSAGetLastError());
            WSACleanup();
            return;
        }

        // Connect to server.
        const int ConnectResultValue = connect(SocketConnection,
            Ptr->ai_addr, (int)Ptr->ai_addrlen);

        // Check for errors. If error, just continue to try and get the
        // next connection
        if (ConnectResultValue == SOCKET_ERROR)
        {
            closesocket(SocketConnection);
            SocketConnection = INVALID_SOCKET;
            continue;
        }

        // If have found a valid connection, break and terminate 
        // processing as we've found a valid connection
        break;
    }
}

bool USocketClientInstance::CloseSocketConnection()
{
    RPES_LOG_INFO(TEXT("Closing socket connection."));

    // Check if connection is valid
    if (SocketConnection == INVALID_SOCKET)
    {
        RPES_LOG_WARNING(TEXT("Socket connection does not exist to be "
            "closed."));
        return false;
    }

    // Shutdown the connection since no more data will be sent
    const int ShutdownResult = shutdown(SocketConnection, SD_SEND);

    // Check for shutdown errors
    if (ShutdownResult == SOCKET_ERROR)
    {
        RPES_LOG_ERROR(TEXT("Shutdown failed with error: %d"),
            WSAGetLastError());
    }

    // Close socket
    closesocket(SocketConnection);

    // Cleanup winsock
    WSACleanup();

    // Set socket connetion to invalid
    SocketConnection = INVALID_SOCKET;

    return true;
}

FString USocketClientInstance::SendMessageAndGetResponse(const char* Message)
{
    RPES_LOG_INFO(TEXT("Sending message to server."));

    // Check if connection is valid
    if (!IsConnectionValid())
    {
        RPES_LOG_ERROR(TEXT("Could not send message as socket connection is \
            not valid."));
        return FString();
    }

    // Sending message
    const int SendReturn = send(SocketConnection, Message, 
        (int)strlen(Message), 0);

    // Check for error
    if (SendReturn == SOCKET_ERROR)
    {
        RPES_LOG_ERROR(TEXT("Send failed with error: %d"), WSAGetLastError());

        // Cleanup winsock
        closesocket(SocketConnection);
        WSACleanup();

        // Set socket connetion to invalid
        SocketConnection = INVALID_SOCKET;

        return FString();
    }

    // Setup buffer length to receive message. 
    // Any more characters will be discarded
    char Recvbuf[DEFAULT_BUFLEN];
    const int Recvbuflen = DEFAULT_BUFLEN;

    // Final received message
    FString FinalReceivedMessage = FString();

    while (true)
    {
        RPES_LOG_INFO(TEXT("Awaiting server response..."));

        // Await response from socket 
        // (this will stall the game thread until we receive a response)
        const int ReceiveReturn = recv(SocketConnection, Recvbuf,
            Recvbuflen, 0);

        // Check for errors
        if (ReceiveReturn < 0)
        {
            RPES_LOG_ERROR(TEXT("Recv failed with error: %d"),
                WSAGetLastError());
            return FString();
        }

        // Debug amount of bytes received
        RPES_LOG_INFO(TEXT("Bytes received: %d"), ReceiveReturn);

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
