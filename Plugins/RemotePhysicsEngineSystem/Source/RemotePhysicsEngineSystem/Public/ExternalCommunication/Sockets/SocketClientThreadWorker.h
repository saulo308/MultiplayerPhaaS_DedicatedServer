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
class REMOTEPHYSICSENGINESYSTEM_API FSocketClientThreadWorker :
    public FRunnable
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
        Response(FString()), bIsRunning(false) {}

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
        bIsRunning = false;
        MessageToSend.Empty();
        Response.Empty();
    }
    
    /** 
    * Returns the socket server response. This should only be called after
    * a run() has been called (so it can get the server's response).
    * 
    * @return The socket server response to the message sent
    */
    FString ConsumeResponse() 
    { 
        // Consume the response (set the response to FString so we can run
        // the thread again)
        const FString CosumedResponse = Response;
        Response = FString();

        return CosumedResponse;
    }

    /** 
    * Sets the message to send to the socket server.
    * 
    * @param InMessageToSend The message to send to the socket server once this
    * runnable object runs
    */
    void SetMessageToSend(const FString& InMessageToSend)
       { MessageToSend = InMessageToSend; }

    /** */
    bool HasMessageToSend() const { return !MessageToSend.IsEmpty(); }

    bool HasResponseToConsume() const { return !Response.IsEmpty(); }

    /** */
    void StartThread() { bIsRunning = true; }

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

    /** */
    bool bIsRunning = false;
};
