// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.


#include "ExternalCommunication/Sockets/SocketClientThreadWorker.h"
#include "ExternalCommunication/Sockets/SocketClientProxy.h"

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

    // Set the flag to false so it can not be run again except if the flag
    // is toggled
    bShouldRun = false;

    // Convert message to std string
    std::string MessageAsStdString(TCHAR_TO_UTF8(*MessageToSend));

    // Convert message to char*. This is needed as some UE converting has 
    // the limitation of 128 bytes, returning garbage when it's over it
    char* MessageAsChar = &MessageAsStdString[0];

    // Send the message and get the response
    Response = FSocketClientProxy::SendMessageAndGetResponse(MessageAsChar, 
        ServerId);

    return 0;
}
