#pragma once

#include "socket.h"
#include <memory>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace Mantids29 { namespace Network { namespace Sockets {

class Socket_DatagramBase : public Socket
{
public:
    struct Block
    {
        Block()
        {
            m_data = nullptr;
            m_dataLength = -1;
        }
        ~Block()
        {
            this->free();
        }
        void free()
        {
            if (m_data) delete [] m_data;
        }
        void copy(void * _data, int dlen)
        {
            if (dlen>0 && dlen<1024*1024) // MAX: 1Mb.
            {
                this->free();
                m_data = new unsigned char[dlen];
                memcpy(m_data,_data,dlen);
            }
        }
        struct sockaddr m_socketAddress;
        unsigned char * m_data;
        int m_dataLength;
    };

    Socket_DatagramBase();
    virtual ~Socket_DatagramBase();

    // Datagram Specific Functions.
    virtual std::shared_ptr<Block> readBlock() = 0;

    // Socket specific functions:
    virtual bool isConnected() = 0;
    virtual bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t &recvbuffer = 0, const int32_t &backlog = 10)  = 0;
    virtual bool connectFrom(const char * bindAddress,const char * remoteHost, const uint16_t & port, const uint32_t &timeout = 30) = 0;
    virtual bool writeBlock(const void * data, const uint32_t &datalen) = 0;
    virtual bool readBlock(void * data, const uint32_t & datalen) = 0;
};

typedef std::shared_ptr<Socket_DatagramBase> Socket_DatagramBase_SP;

}}}
