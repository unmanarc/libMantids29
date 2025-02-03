#pragma once

#include "socket.h"
#include <memory>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace Mantids30 { namespace Network { namespace Sockets {

class Socket_DatagramBase : public Socket
{
public:
    struct Block
    {
        Block()
        {
            data = nullptr;
            dataLength = -1;
        }
        ~Block()
        {
            this->free();
        }
        void free()
        {
            if (data) delete [] data;
        }
        void copy(void * _data, int dlen)
        {
            if (dlen>0 && dlen<1024*1024) // MAX: 1Mb.
            {
                this->free();
                data = new unsigned char[dlen];
                memcpy(data,_data,dlen);
            }
        }
        struct sockaddr socketAddress;
        unsigned char * data;
        int dataLength;
    };

    Socket_DatagramBase() = default;
    virtual ~Socket_DatagramBase() = default;

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
