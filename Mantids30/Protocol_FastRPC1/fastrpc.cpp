#include "fastrpc.h"
#include <Mantids30/Threads/lock_shared.h>
#include <memory>

using namespace Mantids30::Network::Protocols::FastRPC;
using namespace Mantids30;
using Ms = std::chrono::milliseconds;
using S = std::chrono::seconds;



void fastRPCPingerThread( FastRPC1 * obj )
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "fRPC1:Pings");
#endif

    while (obj->waitPingInterval())
    {
        obj->sendPings(); // send pings to every registered client...
    }
}

FastRPC1::FastRPC1(uint32_t threadsCount, uint32_t taskQueues)
{
    m_threadPool = new Mantids30::Threads::Pool::ThreadPool(threadsCount, taskQueues);

    m_finished = false;
    m_overwriteContext = nullptr;

    setRWTimeout();
    setPingInterval();
    setRemoteExecutionTimeoutInMS();
    setMaxMessageSize();
    setQueuePushTimeoutInMS();
    setRemoteExecutionDisconnectedTries();

    m_threadPool->start();
    m_pinger = std::thread(fastRPCPingerThread,this);
}

FastRPC1::~FastRPC1()
{
    // Set pings to cease (the current one will continue)
    m_finished=true;

    // Notify that pings should stop now... This can take a while (while pings are cycled)...
    {
        std::unique_lock<std::mutex> lk(m_pingMutex);
        m_pingCond.notify_all();
    }

    // Wait until the loop ends...
    m_pinger.join();

    delete m_threadPool;
}

void FastRPC1::stop()
{
    m_threadPool->stop();
}

void FastRPC1::setQueuePushTimeoutInMS(const uint32_t &value)
{
    this->m_queuePushTimeoutInMS = value;
}

bool FastRPC1::addMethod(const std::string &methodName, const FastRPC1::Method &method)
{
    Threads::Sync::Lock_RW lock(m_methodsMutex);
    if (m_methods.find(methodName) == m_methods.end() )
    {
        // Put the method.
        m_methods[methodName] = method;
        return true;
    }
    return false;
}

void FastRPC1::sendPings()
{
    // This will create some traffic:
    auto keys = m_connectionsByKeyId.getKeys();
    for (const auto & i : keys)
    {
        // Avoid to ping more hosts during program finalization...
        if (m_finished)
            return;
        // Run unexistant remote function
        runRemoteRPCMethod(i,"_pingNotFound_",{},nullptr,false);
    }
}

void FastRPC1::setPingInterval(uint32_t _intvl)
{
    m_pingIntvl = _intvl;
}

uint32_t FastRPC1::getPingInterval()
{
    return m_pingIntvl;
}

bool FastRPC1::waitPingInterval()
{
    std::unique_lock<std::mutex> lk(m_pingMutex);
    if (m_pingCond.wait_for(lk,S(m_pingIntvl)) == std::cv_status::timeout )
    {
        return true;
    }
    return false;
}

json FastRPC1::runLocalRPCMethod(const std::string &methodName, const std::string &connectionKey, const std::string & data, std::shared_ptr<void> context, const json & payload, bool *found)
{
    json r;
    Threads::Sync::Lock_RD lock(m_methodsMutex);
    if (m_methods.find(methodName) != m_methods.end())
    {
        r = m_methods[methodName].method(m_overwriteContext?m_overwriteContext:m_methods[methodName].context
                                          ,connectionKey,payload,context,data);
        if (found) *found =true;
    }
    else
    {
        if (found) *found =false;
    }
    return r;
}

void FastRPC1::eventUnexpectedAnswerReceived(FastRPC1::Connection *, const std::string & )
{
}

int FastRPC1::processAnswer(FastRPC1::Connection * connection)
{
    uint32_t maxAlloc = m_maxMessageSize;
    uint64_t requestId;
    uint8_t executionStatus;
    char * payloadBytes;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=connection->stream->readU<uint64_t>();
    if (!requestId)
    {
        return -1;
    }
    ////////////////////////////////////////////////////////////
    // READ IF EXECUTED.
    executionStatus=connection->stream->readU<uint8_t>();

    // READ THE PAYLOAD...
    payloadBytes = connection->stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    if (true)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        if ( connection->pendingRequests.find(requestId) != connection->pendingRequests.end())
        {
            connection->executionStatus[requestId] = executionStatus;

            Mantids30::Helpers::JSONReader2 reader;
            bool parsingSuccessful = reader.parse( payloadBytes, connection->answers[requestId] );
            if (parsingSuccessful)
            {
                // Notify that there is a new answer... everyone have to check if it's for him.
                connection->cvAnswers.notify_all();
            }
            else
            {
                // if not able to answer, please remove from answers...
                // TODO: notify this malformed data...
                connection->answers.erase(requestId);
                connection->executionStatus.erase(requestId);
                connection->pendingRequests.erase(requestId);
            }
        }
        else
        {
            eventUnexpectedAnswerReceived(connection, payloadBytes );
        }
    }

    delete [] payloadBytes;
    return 1;
}

int FastRPC1::processQuery(std::shared_ptr<Sockets::Socket_Stream> stream, const std::string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket, std::shared_ptr<void> context, const std::string &data)
{
    uint32_t maxAlloc = m_maxMessageSize;
    uint64_t requestId;
    char * payloadBytes;
    bool ok;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=stream->readU<uint64_t>();
    if (!requestId)
    {
        return -1;
    }
    // READ THE METHOD NAME.
    std::string methodName = stream->readStringEx<uint8_t>(&ok);
    if (!ok)
    {
        return -2;
    }
    // READ THE PAYLOAD...
    payloadBytes = stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    // Process / Inject task:
    Mantids30::Helpers::JSONReader2 reader;
    std::shared_ptr<FastRPC1::ThreadParameters> params = std::make_shared<FastRPC1::ThreadParameters>();
    params->requestId = requestId;
    params->methodName = methodName;
    params->done = mtDone;
    params->mtSocket = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->key = key;
    params->data = data;
    params->context = context;
    params->maxMessageSize = m_maxMessageSize;

    bool parsingSuccessful = reader.parse( payloadBytes, params->payload );
    delete [] payloadBytes;

    if ( !parsingSuccessful )
    {
        // Bad Incoming JSON... Disconnect
        //delete params;
        return -3;
    }
    else
    {
        params->done->lockShared();
        if (!m_threadPool->pushTask(executeRPCTask,params,m_queuePushTimeoutInMS,priority,key))
        {
            // Can't push the task in the queue. Null answer.
            eventFullQueueDrop(params.get());
            sendRPCAnswer(params.get(),"", EXEC_STATUS_ERR_REMOTE_QUEUE_OVERFLOW);
            params->done->unlockShared();
            //delete params;
        }
    }
    return 1;
}

std::shared_ptr<void> FastRPC1::getOverwriteObject() const
{
    return m_overwriteContext;
}

void FastRPC1::setOverwriteObject(std::shared_ptr<void> newOverwriteObject)
{
    m_overwriteContext = newOverwriteObject;
}

uint32_t FastRPC1::getRWTimeout() const
{
    return m_rwTimeout;
}

void FastRPC1::setRWTimeout(uint32_t _rwTimeout)
{
    m_rwTimeout = _rwTimeout;
}

void FastRPC1::setRemoteExecutionDisconnectedTries(const uint32_t &value)
{
    m_remoteExecutionDisconnectedTries = value;
}

void FastRPC1::setRemoteExecutionTimeoutInMS(const uint32_t &value)
{
    m_remoteExecutionTimeoutInMS = value;
}

int FastRPC1::processConnection(std::shared_ptr<Sockets::Socket_Stream> stream, const std::string &key, const FastRPC1::CallBackOnConnected &_cb_OnConnected, const float &keyDistFactor, std::shared_ptr<void> context, const std::string &data)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "FRPC:procCNT");
#endif

    int ret = 1;

    Threads::Sync::Mutex_Shared mtDone;
    Threads::Sync::Mutex mtSocket;

    FastRPC1::Connection * connection = new FastRPC1::Connection;
    connection->context = context;
    connection->data = data;
    connection->mtSocket = &mtSocket;
    connection->key = key;
    connection->stream = stream;

    if (!m_connectionsByKeyId.addElement(key,connection))
    {
        delete connection;
        return -2;
    }

    // Now here is connected....
    if (_cb_OnConnected.fastRPCCB_OnConnected != nullptr)
    {
        auto i = std::thread(_cb_OnConnected.fastRPCCB_OnConnected, key, _cb_OnConnected.context);
        i.detach();
    }

    stream->setReadTimeout(m_rwTimeout);
    stream->setWriteTimeout(m_rwTimeout);

    while (ret>0)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        bool readOK;
        switch (stream->readU<uint8_t>(&readOK))
        {
        case 'A':
            // Process Answer
            ret = processAnswer(connection);
            break;
        case 'Q':
            // Process Query
            ret = processQuery(stream,key,keyDistFactor,&mtDone,&mtSocket,context,data);
            break;
        case 0:
            // Remote shutdown
            // TODO: clean up on exit and send the signal back?
            if (!readOK) // <- read timeout.
                ret = -101;
            else
                ret = 0;
            break;
        default:
            // Invalid protocol.
            ret = -100;
            break;
        }
//        connection->lastReceivedData = time(nullptr);
    }

    // Wait until all task are done.
    mtDone.lock();
    mtDone.unlock();

    stream->shutdownSocket();

    connection->terminated = true;
    connection->cvAnswers.notify_all();

    m_connectionsByKeyId.destroyElement(key);

    return ret;
}

void FastRPC1::executeRPCTask(std::shared_ptr<void> taskData)
{
    FastRPC1::ThreadParameters * params = (FastRPC1::ThreadParameters *)(taskData.get());

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";

    bool found=false;
    json r = ((FastRPC1 *)params->caller)->runLocalRPCMethod(params->methodName,params->key,params->data,params->context,params->payload,&found);
    std::string output = Json::writeString(builder, r);
    sendRPCAnswer(params,output,found? EXEC_STATUS_SUCCESS : EXEC_STATUS_ERR_METHOD_NOT_FOUND);
    params->done->unlockShared();
}

void FastRPC1::sendRPCAnswer(FastRPC1::ThreadParameters *params, const std::string &answer, uint8_t executionStatus)
{
    // Send a block.
    params->mtSocket->lock();
    if (    params->streamBack->writeU<uint8_t>('A') && // ANSWER
            params->streamBack->writeU<uint64_t>(params->requestId) &&
            params->streamBack->writeU<uint8_t>(executionStatus) &&
            params->streamBack->writeStringEx<uint32_t>(answer.size()<=params->maxMessageSize?answer:"",params->maxMessageSize ) )
    {
    }
    params->mtSocket->unlock();
}

void FastRPC1::setMaxMessageSize(const uint32_t &value)
{
    m_maxMessageSize = value;
}

json FastRPC1::runRemoteRPCMethod(const std::string &connectionKey, const std::string &methodName, const json &payload, json *error, bool retryIfDisconnected)
{
    json r;

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::string output = Json::writeString(builder, payload);

    if (output.size()>m_maxMessageSize)
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 1;
            (*error)["errorMessage"] = "Payload exceed the Maximum Message Size.";
        }
        return r;
    }

    FastRPC1::Connection * connection;

    uint32_t _tries=0;
    while ( (connection=(FastRPC1::Connection *)m_connectionsByKeyId.openElement(connectionKey))==nullptr )
    {
        _tries++;
        if (_tries >= m_remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            eventRemotePeerDisconnected(connectionKey,methodName,payload);
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 2;
                (*error)["errorMessage"] = "Abort after remote peer not found/connected.";
            }
            return r;
        }
        sleep(1);
    }

    uint64_t requestId;
    // Create a request ID.
    connection->mtReqIdCt.lock();
    requestId = connection->requestIdCounter++;
    connection->mtReqIdCt.unlock();

    if (1)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        // Create authorization to be inserted:
        connection->pendingRequests.insert(requestId);
    }

    connection->mtSocket->lock();
    if (    connection->stream->writeU<uint8_t>('Q') && // QUERY FOR ANSWER
            connection->stream->writeU<uint64_t>(requestId) &&
            connection->stream->writeStringEx<uint8_t>(methodName) &&
            connection->stream->writeStringEx<uint32_t>( output,m_maxMessageSize ) )
    {
    }
    connection->mtSocket->unlock();

    // Time to wait for answers...
    for (;;)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);

        // Process multiple signals until our answer comes...

        if (connection->cvAnswers.wait_for(lk,Ms(m_remoteExecutionTimeoutInMS)) == std::cv_status::timeout )
        {
            // break by timeout. (no answer)
            eventRemoteExecutionTimedOut(connectionKey,methodName,payload);
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 3;
                (*error)["errorMessage"] = "Remote Execution Timed Out: No Answer Received.";
            }
            break;
        }

        if ( lk.owns_lock() && connection->answers.find(requestId) != connection->answers.end())
        {
            // break by element found. (answer)
            uint8_t executionStatus = connection->executionStatus[requestId];
            r = connection->answers[requestId];
            if (error)
            {
                switch (executionStatus)
                {
                case 2:
                    (*error)["succeed"] = true;
                    (*error)["errorId"] = 0;
                    (*error)["errorMessage"] = "Execution OK.";
                    break;
                case 3:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 4;
                    (*error)["errorMessage"] = "Remote Execution Failed: Full Queue.";
                    break;
                case 4:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 5;
                    (*error)["errorMessage"] = "Remote Execution Failed: Method Not Found.";
                    break;
                default:
                    (*error)["succeed"] = false;

                }
            }
            break;
        }

        if ( lk.owns_lock() && connection->terminated )
        {
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 6;
                (*error)["errorMessage"] = "Connection is terminated: No Answer Received.";
            }
            break;
        }
    }

    if (1)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        // Revoke authorization to be inserted, clean results...
        connection->answers.erase(requestId);
        connection->executionStatus.erase(requestId);
        connection->pendingRequests.erase(requestId);
    }

    m_connectionsByKeyId.releaseElement(connectionKey);

    if (error)
    {
        if (!error->isMember("succeed"))
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 99;
            (*error)["errorMessage"] = "Unknown Error.";
        }
    }

    return r;
}

bool FastRPC1::runRemoteClose(const std::string &connectionKey)
{
    bool r = false;

    FastRPC1::Connection * connection;
    if ((connection=(FastRPC1::Connection *)m_connectionsByKeyId.openElement(connectionKey))!=nullptr)
    {

        connection->mtSocket->lock();
        if (    connection->stream->writeU<uint8_t>(0) )
        {
        }
        connection->mtSocket->unlock();

        m_connectionsByKeyId.releaseElement(connectionKey);
    }
    else
    {
        eventRemotePeerDisconnected(connectionKey,"",{});
    }
    return r;
}

std::set<std::string> FastRPC1::getConnectionKeys()
{
    return m_connectionsByKeyId.getKeys();
}

bool FastRPC1::checkConnectionKey(const std::string &connectionKey)
{
    return m_connectionsByKeyId.isMember(connectionKey);
}

void FastRPC1::eventFullQueueDrop(FastRPC1::ThreadParameters *)
{
}

void FastRPC1::eventRemotePeerDisconnected(const std::string &, const std::string &, const json &)
{
}

void FastRPC1::eventRemoteExecutionTimedOut(const std::string &, const std::string &, const json &)
{

}
