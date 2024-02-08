// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ExternalCommunication/Sockets/SocketClientThreadWorker.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"
#include "ExternalCommunication/Sockets/SocketClientInstance.h"
#include "RemotePhysicsEngineSystem/RemotePhysicsEngineSystemLogging.h"
#include <string>

uint32 FSocketClientThreadWorker::Run()
{
    // If thread should not run, just ignore the call (this is useful for the
    // first call this thread makes upon creation - as we don't want it to be
    // called)
    if (!bShouldRun)
    {
        return 0;
    }

    RPES_LOG_WARNING(TEXT("Sending step."));

    // Set the flag to false so it can not be run again except if the flag
    // is toggled
    bShouldRun = false;

    // Get the socket connection instance to send the message
    const auto* SocketConnectionToSend =
        FSocketClientProxy::GetSocketConnectionByServerId(ServerId);

    // Check if valid 
    if (!SocketConnectionToSend)
    {
        RPES_LOG_ERROR(TEXT("Could not send message to socket with ID \"%d\" "
            "as such connection does not exist."), ServerId);
        return 0;
    }

    // Convert message to send to std::string
    std::string MessageAsStdString(TCHAR_TO_UTF8(*MessageToSend));

    // Convert message to char*. This is needed as some UE converting has 
    // the limitation of 128 bytes, returning garbage when it's over it
    char* MessageAsChar = &MessageAsStdString[0];

    // Send the message and get the response
    //Response = SocketConnectionToSend->SendMessageAndGetResponse
       // (MessageAsChar);

    RPES_LOG_WARNING(TEXT("Got response."));

    return 0;
}
