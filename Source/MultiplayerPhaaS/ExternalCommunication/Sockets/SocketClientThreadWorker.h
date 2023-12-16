// 2023 Copyright Saulo Soares, Brazil. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

/**
* This class is responsible for implementing a worker thread that communicates
* with a given socket server. This is done by calling the FScoketClientProxy
* class. During gameplay, multiple worker threads may be created to communicate
* with multiple servers in paralel.
* 
* @see FSocketClientProxy
*/
class MULTIPLAYERPHAAS_API FSocketClientThreadWorker : public FRunnable
{
public:
    /** 
    * The default constructor. Receives the server id that this worker thread
    * will communicate with.
    * 
    * @param InServerId The socket server id that this worker thread will be
    * communicating with
    */
    FSocketClientThreadWorker(int32 InServerId)
        : MessageToSend(""), ServerId(InServerId),
        Response(FString()), bShouldRun(false) {}

    /**
    * Initializes the runnable object. Called once the thread starts working.
    *
    * @return True if initialization was successful, false otherwise
    */
    virtual bool Init() override { return true; }

    /**
    * Runs the runnable object.
    *
    * This is where all per object thread work is done. This is only called 
    * if the initialization was successful.
    *
    * @return The exit code of the runnable object
    */
    virtual uint32 Run() override;

    /**
    * Stops the runnable object.
    *
    * This is called if a thread is requested to terminate early.
    */
    virtual void Stop() override 
    {
        bShouldRun = false;
        MessageToSend.Empty();
        Response.Empty();
    }
    
    /** 
    * Returns the socket server response. This should only be called after
    * a run() has been called (so it can get the server's response).
    * 
    * @return The socket server response to the message sent
    */
    FString GetResponse() const { return Response; }

    /** 
    * Sets the message to send to the socket server.
    * 
    * @param InMessageToSend The message to send to the socket server once this
    * runnable object runs
    */
    void SetMessageToSend(const FString& InMessageToSend)
        { MessageToSend = InMessageToSend; }

    /** 
    * Toggles the "bShouldRun" flag. This should be toggled before calling 
    * the "run()" method to avoid it being called more than once and also avoid
    * it being called upon thread creation
    */
    void ToggleShouldRun() { bShouldRun = !bShouldRun; }

private:
    /** The message to send the socket server once run() is called. */
    FString MessageToSend = FString();

    /** 
    * The server id that this worker will send its message to. The server id
    * should exist on the FSocketClinetProxy connections map
    */
    int32 ServerId = 0;

    /** The socket server's response to the message sent */
    FString Response = FString();

    /** 
    * Flag that indicates if the message should be sent. I.e., indicates if
    * the run() method can be called. Used to avoid multiple calls at once and
    * avoid the run() method right after the thread creation
    */
    bool bShouldRun = false;
};
