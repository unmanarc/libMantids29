#pragma once

#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <limits>

namespace Mantids29 { namespace Memory { namespace Streams {

/**
 * StreamableObject base class
 * This is a base class for streamable objects that can be retrieved or parsed trough read/write functions.
 */
class StreamableObject
{
public:

    struct Status {

        Status()
        {
            bytesWritten = 0;
            succeed = true;
            finish = false;
        }

        Status(const uint64_t & x)
        {
            bytesWritten = x;
            succeed = true;
            finish = false;
        }

        Status & operator=(const uint64_t & x)
        {
            bytesWritten = x;
            return *this;
        }

        Status & operator+=(const uint64_t & x)
        {
            bytesWritten += x;
            return *this;
        }

        Status & operator+=(const Status & x)
        {
            bytesWritten += x.bytesWritten;
            if (!x.succeed) succeed = false;
            if (x.finish) finish = x.finish;
            return *this;
        }

        bool succeed, finish;
        uint64_t bytesWritten;
    };

    StreamableObject();
    virtual ~StreamableObject();

    // TODO: what if the protocol reached std::numeric_limits<uint64_t>::max() ? enforce 64bit max. (on streamTo)
    // TODO: report percentage completed

    virtual std::string toString();

    /**
     * @brief writeEOF proccess the end of the stream (should be implemented on streamTo)
     */
    virtual void writeEOF(bool);
    /**
     * @brief size return the size of the sub-container if it's fixed size.
     * @return std::numeric_limits<uint64_t>::max() if the stream is not fixed size
     */
    virtual uint64_t size() const { return std::numeric_limits<uint64_t>::max(); }
    virtual bool streamTo(Memory::Streams::StreamableObject * out, Status & wrStatUpd)=0;
    virtual Status write(const void * buf, const size_t &count, Status & wrStatUpd)=0;

    Status writeFullStream(const void *buf, const size_t &count, Status & wrStatUpd);
    /**
     * @brief writeStream Write into stream using std::strings
     * @param buf data to be streamed.
     * @return true if succeed (all bytes written)
     */
    Status writeString(const std::string & buf, Status & wrStatUpd);
    Status writeString(const std::string & buf);

    Status strPrintf(const char * format, ...);
    Status strPrintf(Status & wrStatUpd, const char * format, ...);

    uint16_t getFailedWriteState() const;

protected:
    bool setFailedWriteState(const uint16_t &value = 1);
    uint16_t failedWriteState;

#ifdef _WIN32
private:
    static int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

};

}}}



