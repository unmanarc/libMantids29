#ifndef SOCKET_UNIX_H
#define SOCKET_UNIX_H

#ifndef _WIN32
#include "socket_stream_base.h"

namespace Mantids29 { namespace Network { namespace Sockets {


/**
 * Unix Socket Class
 */
class  Socket_UNIX : public Sockets::Socket_Stream_Base {
public:
	/**
	 * Class constructor.
	 */
    Socket_UNIX();
    /**
     * @brief listenOn Listen on an specific path and address
     * @param path Unix Path
     * @param recvbuffer size in bytes of recv buffer.
     * @param backlog connection backlog of unattended incomming connections.
     * @return true if listening
     */
    bool listenOn(const char * path, const int32_t & recvbuffer = 0, const int32_t &backlog = 10);
    bool listenOn(const uint16_t &, const char * path, const int32_t & recvbuffer = 0, const int32_t &backlog = 10) override;
    /**
     * Connect to remote host using an UNIX socket.
     * @param path local path to connect to.
     * @param port 16-bit unsigned integer with the remote port
     * @param timeout timeout in seconds to desist the connection.
     * @return true if successfully connected
     */
    bool connectFrom(const char *,const char * path, const uint16_t &, const uint32_t & timeout = 30) override;
    /**
     * Accept a new connection on a listening socket.
     * @return returns a socket with the new connection.
     */
    Socket_Stream_Base *acceptConnection() override;
};

typedef std::shared_ptr<Socket_UNIX> Socket_UNIX_SP;
}}}

#endif
#endif // SOCKET_UNIX_H
