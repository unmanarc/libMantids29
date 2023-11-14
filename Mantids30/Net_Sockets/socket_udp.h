#pragma once

#include "socket_datagrambase.h"

#ifndef _WIN32
#include <netdb.h>
#endif

namespace Mantids30 { namespace Network { namespace Sockets {

class Socket_UDP : public Socket_DatagramBase {
public:
    /**
     * Class constructor
     */
    Socket_UDP();
    /**
     * Class destructor
     */
    ~Socket_UDP() override;
    /**
     * Check if the remote pair is connected or not.
     * @param true if is it connected
     */
    bool isConnected() override;
    /**
     * Listen on an specific UDP port and address
     * @param listenOnAddress address to listen on. (use :: for ipv6 or 0.0.0.0 if ipv4)
     * @param port 16-bit unsigned integer with the listening UDP port (0-65535)
     * @return true if we can bind the port.
     */
    bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t &recvbuffer = 0, const int32_t &backlog = 10) override;
    /**
     * Setup parameters of UDP remote pair.
     * This connect function does not connect at all. It only set the socket and the parameters to send the information.
     * @param remoteHost remote hostname to connect to
     * @param port 16-bit unsigned integer with the remote port
     * @param timeout timeout in seconds to desist the connection.
     * @return true if successfully connected
     */
    bool connectFrom(const char * bindAddress, const char * remoteHost, const uint16_t & port, const uint32_t & timeout = 30) override;
    /**
     * Write a datagram on the socket
     * Only datagrams below 65507 bytes are valid to be sent here.
     * @param data datagram data.
     * @param datalen data length in bytes of the datagram
     * @return true if the datagram was sucessfully released by our software.
     */
    bool writeBlock(const void * data, const uint32_t & datalen) override;
    /**
     * Read a datagram from the socket
     * Receive the data block of maximum 65507 bytes.
     * You may want to specify read a timeout for your protocol.
     * The data written is a composed message with the sockaddr, message size and
     * @param data input variable to write data into. (sockaddr (with source data) + answer int (usually bytes received or error) + data).
     * @param datalen maximum data length in bytes (you should consider to use 65507+sizeof(int)+sizeof(sockaddr).
     * @return true if the data block was sucessfully received.
     */
    bool readBlock(void *, const uint32_t &) override;

    /**
     * Read a datagram from the socket
     * Receive the data block of maximum 65507 bytes.
     * You may want to specify read a timeout for your protocol.
     */
    std::shared_ptr<Block> readBlock() override;

    /**
     * Minimum read size allowed on read funcion.
     */
    uint32_t getMinReadSize();

private:
    void freeAddrInfo();
    addrinfo *m_addressInfoResolution;
};

typedef std::shared_ptr<Socket_UDP> Socket_UDP_SP;

}}}
