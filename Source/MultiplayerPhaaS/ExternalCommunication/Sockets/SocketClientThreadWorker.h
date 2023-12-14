// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

/**
* 
*/
class MULTIPLAYERPHAAS_API FSocketClientThreadWorker : public FRunnable
{
public:
    /** */
    FSocketClientThreadWorker(int32 InServerId)
        : MessageToSend(""), ServerId(InServerId),
        Response(FString()), bShouldRun(false) {}

    /** */
    virtual bool Init() override { return true; }

    /** */
    virtual uint32 Run() override;

    /** */
    virtual void Stop() override 
    {
        bShouldRun = false;
        MessageToSend.Empty();
        Response.Empty();
    }
    
    /** */
    FString GetResponse() const { return Response; }

    /** */
    void SetMessageToSend(const FString& InMessageToSend)
        { MessageToSend = InMessageToSend; }

    /** */
    void ToggleShouldRun() { bShouldRun = !bShouldRun; }

private:
    /** */
    FString MessageToSend = FString();

    /** */
    int32 ServerId = 0;

    /** */
    FString Response = FString();

    /** */
    bool bShouldRun = false;
};
