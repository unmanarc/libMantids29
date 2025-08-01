#pragma once

#include "socket_stream.h"
#include "streams_bridge_thread.h"

#include <cstdint>
#include <stdint.h>

#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace Mantids30 { namespace Network { namespace Sockets { namespace NetStreams {


/**
 * @brief The Bridge class connect two pipe stream sockets.
 */
class Bridge
{
public:
    enum TransmitionMode {
        TRANSMITION_MODE_STREAM=0,
        TRANSMITION_MODE_CHUNKSANDPING=1
    };
    /**
     * @brief Socket_Bridge constructor.
     */
    Bridge();
    /**
     * @brief start, begin the communication between peers in threaded mode.
     * @param autoDelete true (default) if going to delete the whole pipe when finish.
     * @return true if initialized, false if not.
     */
    bool start(bool _autoDeleteStreamPipeOnExit = true, bool detach = true);
    /**
     * @brief wait will block-wait until thread finishes
     * @return -1 failed, 0: socket 0 closed the connection, 1: socket 1 closed the connection.
     */
    int wait();

    /**
     * @brief process, begin the communication between peers blocking until it ends.
     * @return -1 failed, 0: socket 0 closed the connection, 1: socket 1 closed the connection.
     */
    int process();
    /**
     * @brief processPeer, begin the communication between peer i to the next peer.
     * @param i peer number (0 or 1)
     * @return true if transmitted and closed, false if failed.
     */
    bool processPeer(Side i);
    /**
     * @brief SetPeer Set Stream Socket Peer (0 or 1)
     * @param i peer number: 0 or 1.
     * @param s peer established socket.
     * @return true if peer setted successfully.
     */
    bool setPeer(Side i, std::shared_ptr<Sockets::Socket_Stream>  s);
    /**
     * @brief GetPeer Get the Pipe Peers
     * @param i peer number (0 or 1)
     * @return Stream Socket Peer.
     */
    std::shared_ptr<Sockets::Socket_Stream>  getPeer(Side i);
    /**
     * @brief setAutoDelete Auto Delete the pipe object when finish threaded job.
     * @param value true for autodelete (default), false for not.
     */
    void setAutoDeleteStreamPipeOnThreadExit(bool value = true);
    /**
     * @brief shutdownRemotePeer set to shutdown both sockets peer on finish.
     * @param value true for close the remote peer (default), false for not.
     */
    void setToShutdownRemotePeer(bool value = true);
    /**
     * @brief closeRemotePeer set to close both sockets peer on finish.
     * @param value true for close the remote peer (default), false for not.
     */
    void setToCloseRemotePeer(bool value = true);
    /**
     * @brief getSentBytes Get bytes transmitted from peer 0 to peer 1.
     * @return bytes transmitted.
     */
    size_t getSentBytes() const;
    /**
     * @brief getRecvBytes Get bytes  transmitted from peer 1 to peer 0.
     * @return bytes transmitted.
     */
    size_t getRecvBytes() const;
    /**
     * @brief isAutoDeleteStreamPipeOnThreadExit Get if this class autodeletes when pipe is over.
     * @return true if autodelete is on.
     */
    bool isAutoDeleteStreamPipeOnThreadExit() const;
    /**
     * @brief isAutoDeleteSocketsOnExit Get if pipe endpoint sockets are going to be deleted when this class is destroyed.
     * @return true if it's going to be deleted.
     */
    bool isAutoDeleteSocketsOnExit() const;
    /**
     * @brief setAutoDeleteSocketsOnExit Set if pipe endpoint sockets are going to be deleted when this class is destroyed.
     * @param value true if you want sockets to be deleted on exit.
     */
    void setAutoDeleteSocketsOnExit(bool value);
    /**
     * @brief setCustomPipeProcessor Set custom pipe processor.
     * @param value pipe processor.
     */
    void setCustomPipeProcessor(Bridge_Thread *value, bool deleteOnExit = false);

    /**
     * @brief getTransmitionMode Get Transmition Mode
     * @return value with the mode (chunked or streamed)
     */
    TransmitionMode getTransmitionMode() const;
    /**
     * @brief setTransmitionMode set the transmition mode
     * @param newTransmitionMode  value with the mode (chunked or streamed)
     */
    void setTransmitionMode(TransmitionMode newTransmitionMode);
    /**
     * @brief getLastPing Get Last Ping (in unix epoch time)
     * @return last ping
     */
    time_t getLastPing();


    int getLastError(Side side);

    /**
     * @brief sendPing (internal function)
     */
    void sendPing();

    uint32_t getPingEveryMS() const;
    void setPingEveryMS(uint32_t newPingEveryMS);

private:
    static void remotePeerThread(Bridge * stp);
    static void pingThread(Bridge * stp);
    static void pipeThread(Bridge * stp);

    Bridge_Thread *m_bridgeThreadPrc = nullptr;

    std::shared_ptr<Sockets::Socket_Stream>  m_peers[2] = {nullptr, nullptr};
    TransmitionMode m_transmitionMode = TRANSMITION_MODE_STREAM;

    std::atomic<size_t> m_sentBytes{0}, m_recvBytes{0};
    std::atomic<int> m_finishingPeer{-1};
    std::atomic<bool> m_shutdownRemotePeerOnFinish{false};
    std::atomic<bool> m_closeRemotePeerOnFinish{false};

    int m_lastError[2] = {0, 0};

    std::mutex m_lastPingMutex;
    time_t m_lastPingTimestamp{0};

    std::mutex m_endPingLoopMutex;
    std::condition_variable m_endPingCond;

    uint32_t m_pingEveryMS{5000};

    bool m_isPingFinished = false;
    bool m_autoDeleteStreamPipeOnExit = true;
    //bool autoDeleteSocketsOnExit;
    bool m_autoDeleteCustomPipeOnClose = false;

    std::thread m_pipeThreadP;
};

}}}}
