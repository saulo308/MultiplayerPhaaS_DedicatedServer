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
    while (bIsRunning)
    {
        // If there's no message to send, do not run
        if (!HasMessageToSend())
        {

            // Sleep to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            continue;
        }

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

        // Convert message to send to std::string
        std::string MessageAsStdString(TCHAR_TO_UTF8(*MessageToSend));

        // Convert message to char*. This is needed as some UE converting has 
        // the limitation of 128 bytes, returning garbage when it's over it
        char* MessageAsChar = &MessageAsStdString[0];

        // Send the message and get the response
        Response = SocketConnectionToSend->SendMessageAndGetResponse
            (MessageAsChar);

        // Reset the message to send as we already sent it
        MessageToSend = FString();
    }

    return 0;
}
