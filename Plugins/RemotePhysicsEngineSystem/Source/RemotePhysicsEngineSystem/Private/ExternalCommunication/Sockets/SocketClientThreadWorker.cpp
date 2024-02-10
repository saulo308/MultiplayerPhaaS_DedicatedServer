// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ExternalCommunication/Sockets/SocketClientThreadWorker.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "ExternalCommunication/Sockets/SocketClientInstance.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"
#include <string>
#include <thread>
#include <chrono>

uint32 FSocketClientThreadWorker::Run()
{
    // Small delay to initiate setup() and loop() calls
    FPlatformProcess::Sleep(0.03f);

    // While the thread is active, keep running
    while (IsThreadRunning())
    {
        //RPES_LOG_WARNING(TEXT("(%d) Run thread (%d)!"), ServerId, 
           // !HasMessageToSend());

        // If there's no message to send, do not run
        if (!HasMessageToSend())
        {
            //RPES_LOG_WARNING(TEXT("No message to send"));
            // Sleep to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }
        //RPES_LOG_WARNING(TEXT("Send step!"));

        // Get the socket connection instance to send the message
        auto* SocketConnectionToSend =
            FSocketClientProxy::GetSocketConnectionByServerId(ServerId);

        // Check if valid 
        if (!SocketConnectionToSend)
        {
            RPES_LOG_ERROR(TEXT("Could not send message to socket with id "
                "\"%d\" as such connection does not exist."), ServerId);
            return 0;
        }

        const FString MessageToSendToSocket = GetMessageToSend();

        //RPES_LOG_WARNING(TEXT("Sending message: %s"), *MessageToSendToSocket);

        // Convert message to send to std::string
        std::string MessageAsStdString(TCHAR_TO_UTF8(*MessageToSend));

        // Convert message to char*. This is needed as some UE converting has 
        // the limitation of 128 bytes, returning garbage when it's over it
        char* MessageAsChar = &MessageAsStdString[0];

        // Send the message and get the response
        const auto ServerResponse = 
            SocketConnectionToSend->SendMessageAndGetResponse(MessageAsChar);

        // Set the response
        SetResponse(ServerResponse);

        // Reset the message to send as we already sent it
        SetMessageToSend(FString());

       // RPES_LOG_WARNING(TEXT("Got step!"));

        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    return 0;
}
