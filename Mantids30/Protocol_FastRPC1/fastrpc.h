#pragma once

#include <Mantids30/Helpers/json.h>

#include <Mantids30/Threads/threadpool.h>
#include <Mantids30/Threads/mutex_shared.h>
#include <Mantids30/Threads/mutex.h>
#include <Mantids30/Net_Sockets/socket_stream.h>
#include <Mantids30/Threads/map.h>
#include <memory>

namespace Mantids30 { namespace Network { namespace Protocols { namespace FastRPC { 

/**
 * @brief The FastRPC class: Bidirectional client-sync/server-async-thread-pooled no-auth RPC Manager
 */
class FastRPC1
{
public:
    struct CallBackOnConnected
    {
        void (*fastRPCCB_OnConnected)(const std::string &, std::shared_ptr<void> context);
        std::shared_ptr<void> context;
    };

    struct Method
    {
        /**
         * @brief Function pointer.
         */
        json (*method)(std::shared_ptr<void> context, const std::string &key, const json & parameters, std::shared_ptr<void> cntObj, const std::string & cntData);
        /**
         * @brief obj object to pass
         */
        std::shared_ptr<void> context = nullptr;
    };

    struct ThreadParameters
    {
        std::shared_ptr<Sockets::Socket_Stream> streamBack;
        uint32_t maxMessageSize;
        void * caller;
        Threads::Sync::Mutex_Shared * done;
        Threads::Sync::Mutex * mtSocket;
        std::string methodName;
        json payload;
        uint64_t requestId;

        // Accesible objects:
        std::string key;
        std::shared_ptr<void> context;
        std::string data;
    };

    class Connection : public Mantids30::Threads::Safe::MapItem
    {
    public:
        Connection()
        {
            stream = nullptr;
            requestIdCounter = 1;
            terminated = false;
        }
        // Socket
        std::shared_ptr<Sockets::Socket_Stream> stream;
        Threads::Sync::Mutex * mtSocket;
        std::string key;

        // Accesible objects:
        std::shared_ptr<void> context;
        std::string data;

        // Request ID counter.
        uint64_t requestIdCounter;
        Threads::Sync::Mutex mtReqIdCt;

        // Answers:
        std::map<uint64_t,json> answers;
        std::map<uint64_t,uint8_t> executionStatus;
        std::mutex mtAnswers;
        std::condition_variable cvAnswers;
        std::set<uint64_t> pendingRequests;

        // Finalization:
        std::atomic<bool> terminated;
    };

    enum eTaskExecutionStatus {
        EXEC_STATUS_ERR_GENERIC = 1,
        EXEC_STATUS_SUCCESS = 2,
        EXEC_STATUS_ERR_REMOTE_QUEUE_OVERFLOW = 3,
        EXEC_STATUS_ERR_METHOD_NOT_FOUND = 4
    };


    /**
     * @brief FastRPC This class is designed to persist between connections...
     * @param threadsCount
     * @param taskQueues
     */
    FastRPC1(uint32_t threadsCount = 16, uint32_t taskQueues = 24);
    ~FastRPC1();
    /**
     * @brief stop Stop the thread pool.
     */
    void stop();
    /**
     * @brief addMethod Add Method
     * @param methodName Method Name
     * @param method Method function and Object
     */
    bool addMethod(const std::string & methodName, const FastRPC1::Method & method);


    // Ping functions:
    /**
     * @brief sendPings Internal function to send pings to every connected peer
     */
    void sendPings();
    /**
     * @brief waitPingInterval Wait interval for pings
     * @return true if ping interval completed, false if a signal closed the wait interval (eg. FastRPC destroyed)
     */
    bool waitPingInterval();
    /**
     * @brief setPingInterval Set Ping interval
     * @param _intvl ping interval in seconds
     */
    void setPingInterval(uint32_t _intvl=20);
    /**
     * @brief getPingInterval Get ping interval
     * @return ping interval in seconds
     */
    uint32_t getPingInterval();

    /**
     * @brief processConnection Process Connection Stream and manage bidirectional events from each side (Q/A).
     *                          Additional security should be configured at the TLS Connections, like peer validation
     *                          and you may set up any other authentication mechanisms (like api keys) prior to the socket
     *                          management delegation or in each RPC function.
     * @param stream Stream Socket to be handled with this fast rpc protocol.
     * @param key Connection Name, used for running remote RPC methods.
     * @param keyDistFactor threadpool distribution usage by the key (0.5 = half, 1.0 = full, 0.NN = NN%)
     * @param _cb_OnConnected On connect Method to be executed in background (new thread) when connection is accepted/processed.
     * @param context object memory to be passed everywhere in the connection
     * @param data string to be passsed everywhere in the connection
     * @return 0 if remotely shutted down, or negative if connection error happened.
     */
    int processConnection(std::shared_ptr<Sockets::Socket_Stream> stream,
                          const std::string & key,
                          const FastRPC1::CallBackOnConnected & _cb_OnConnected = {nullptr,nullptr},
                          const float & keyDistFactor=1.0,
                          std::shared_ptr<void> context = nullptr,
                          const std::string & data = ""
                          );


    /**
     * @brief processConnection2 Same as processConnection with _cb_OnConnected and keyDistFactor as defaults
     * @param stream
     * @param key
     * @param context
     * @param data
     * @return
     */
    int processConnection2(std::shared_ptr<Sockets::Socket_Stream> stream,
                                  const std::string & key,
                                  std::shared_ptr<void> context = nullptr,
                                  const std::string & data = ""
                                  )
    {
        return  processConnection(stream,
                                  key,
                                  {nullptr,nullptr},
                                  1.0,
                                  context,
                                  data
                                  );
    }

    /**
     * @brief setTimeout Timeout in milliseconds to desist to put the execution task into the threadpool
     * @param value milliseconds, default is 2secs (2000).
     */
    void setQueuePushTimeoutInMS(const uint32_t &value = 2000);
    /**
     * @brief setMaxMessageSize Max JSON Size
     * @param value max bytes for reception/transmition json, default is 10M
     */
    void setMaxMessageSize(const uint32_t &value = 10*1024*1024);
    /**
     * @brief setRemoteExecutionTimeoutInMS Set the remote Execution Timeout for "runRemoteRPCMethod" function
     * @param value timeout in milliseconds, default is 5secs (5000).
     */
    void setRemoteExecutionTimeoutInMS(const uint32_t &value = 5000);
    /**
     * @brief runRemoteRPCMethod Run Remote RPC Method
     * @param connectionKey Connection ID (this class can thread-safe handle multiple connections at time)
     * @param methodName Method Name
     * @param payload Function Payload
     * @param error Error Return
     * @return Answer, or Json::nullValue if answer is not received or if timed out.
     */
    json runRemoteRPCMethod( const std::string &connectionKey, const std::string &methodName, const json &payload , json * error, bool retryIfDisconnected = true );
    /**
     * @brief runRemoteClose Run Remote Close Method
     * @param connectionKey Connection ID (this class can thread-safe handle multiple connections at time)
     * @return Answer, or Json::nullValue if answer is not received or if timed out.
     */
    bool runRemoteClose( const std::string &connectionKey );

    /**
     * @brief getConnectionKeys Get keys from the current connections.
     * @return set of strings containing the unique keys
     */
    std::set<std::string> getConnectionKeys();

    /**
     * @brief checkConnectionKey Check if the given connection key does exist.
     * @param connectionKey connection key
     * @return true if exist, otherwise false.
     */
    bool checkConnectionKey( const std::string &connectionKey );

    //////////////////////////////////////////////////////////
    // For Internal use only:
    json runLocalRPCMethod(const std::string & methodName, const std::string &key, const std::string & data, std::shared_ptr<void> context, const json &payload, bool * found);

    void setRemoteExecutionDisconnectedTries(const uint32_t &value = 10);


    /**
     * @brief getReadTimeout Get R/W Timeout
     * @return W/R timeout in seconds
     */
    uint32_t getRWTimeout() const;
    /**
     * @brief setRWTimeout   This timeout will be setted on processConnection, call this function before that.
     *
     *                       Read timeout defines how long are we going to wait if no-data comes from a RPC connection socket,
     *                       we also send a periodic ping to avoid the connection to be closed due to RPC per-se inactivity, so
     *                       basically we are only going to close due to network issues.
     *
     *                       Write timeout is also important, since we are using blocking sockets, if the peer is full
     *                       just before having a network problem, the writes (including the ping one) may block the pinging process forever.
     *
     * @param _rwTimeout     W/R timeout in seconds (default: 40)
     */
    void setRWTimeout(uint32_t _rwTimeout = 40);


    /**
     * @brief getOverwriteObject
     * @return
     */
    std::shared_ptr<void>getOverwriteObject() const;
    /**
     * @brief setOverwriteObject Set overwrite object for functions
     * @param newOverwriteObject object.
     */
    void setOverwriteObject(std::shared_ptr<void>newOverwriteObject);

protected:
    // TODO pasar a callbacks
    virtual void eventUnexpectedAnswerReceived(FastRPC1::Connection *connection, const std::string &answer);
    virtual void eventFullQueueDrop(FastRPC1::ThreadParameters * params);
    virtual void eventRemotePeerDisconnected(const std::string &connectionKey, const std::string &methodName, const json &payload);
    virtual void eventRemoteExecutionTimedOut(const std::string &connectionKey, const std::string &methodName, const json &payload);

private:
    static void executeRPCTask(std::shared_ptr<void> taskData);
    static void sendRPCAnswer(FastRPC1::ThreadParameters * parameters, const std::string & answer, uint8_t executionStatus);

    int processAnswer(FastRPC1::Connection *connection);
    int processQuery(std::shared_ptr<Sockets::Socket_Stream> stream, const std::string &key, const float &priority, Threads::Sync::Mutex_Shared *mtDone, Threads::Sync::Mutex *mtSocket, std::shared_ptr<void> context, const std::string &data);

    // Stores active connections indexed by a unique key identifier.
    Mantids30::Threads::Safe::Map<std::string> m_connectionsByKeyId;

    // Configuration parameters for RPC behavior.
    // queuePushTimeoutInMS: Timeout (in milliseconds) for queue push operations.
    // maxMessageSize: Maximum allowed message size in bytes.
    // remoteExecutionTimeoutInMS: Timeout (in milliseconds) for remote execution.
    // remoteExecutionDisconnectedTries: Maximum retry attempts for remote execution when disconnected.
    std::atomic<uint32_t> m_queuePushTimeoutInMS;
    std::atomic<uint32_t> m_maxMessageSize;
    std::atomic<uint32_t> m_remoteExecutionTimeoutInMS;
    std::atomic<uint32_t> m_remoteExecutionDisconnectedTries;

    // Map storing available RPC methods.
    // Key: Method name (string).
    // Value: Corresponding method object.
    std::map<std::string, FastRPC1::Method> m_methods;

    // Shared mutex for synchronizing access to the methods map.
    Threads::Sync::Mutex_Shared m_methodsMutex;

    // Thread pool for handling RPC method execution.
    Mantids30::Threads::Pool::ThreadPool * m_threadPool;

    // Background thread responsible for periodic pinging to maintain connection health.
    std::thread m_pinger;

    // Optional context for overwriting specific execution behaviors.
    std::shared_ptr<void> m_overwriteContext;

    // Indicates whether the RPC system is in a finished state.
    std::atomic<bool> m_finished;

    // Ping-related configuration.
    // pingIntvl: Interval (in milliseconds) between ping attempts.
    // rwTimeout: Read/write timeout (in milliseconds) for network operations.
    uint32_t m_pingIntvl;
    uint32_t m_rwTimeout;

    // Mutex for synchronizing ping operations.
    std::mutex m_pingMutex;

    // Condition variable used to signal ping-related events.
    std::condition_variable m_pingCond;

};

}}}}
