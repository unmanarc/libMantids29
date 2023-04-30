#ifndef HTTP_CONTENT_H
#define HTTP_CONTENT_H

#include "common_urlvars.h"

#include <Mantids29/Memory/subparser.h>
#include <Mantids29/Memory/b_base.h>
#include <Mantids29/Protocol_MIME/mime_message.h>
#include <memory>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

class Content : public Memory::Streams::SubParser
{
public:
    enum eDataType {
        CONTENT_TYPE_BIN,
        CONTENT_TYPE_MIME,
        CONTENT_TYPE_URL
    };

    enum eProcessingMode {
        PROCMODE_CHUNK_SIZE,
        PROCMODE_CHUNK_DATA,
        PROCMODE_CHUNK_CRLF,
        PROCMODE_CONTENT_LENGTH,
        PROCMODE_CONNECTION_CLOSE
    };

    enum eTransmitionMode {
        TRANSMIT_MODE_CHUNKS,
        TRANSMIT_MODE_CONTENT_LENGTH,
        TRANSMIT_MODE_CONNECTION_CLOSE
    };

    Content();
    ~Content() override;


    //////////////////////////////////////////////////////////////////
    //  ------------------ STREAMABLE OUTPUT ------------------
    //////////////////////////////////////////////////////////////////
    /**
     * @brief stream Stream the output to upStream
     * @param wrStat write status
     * @return true if written
     */
    bool stream(Memory::Streams::StreamableObject::Status & wrStat) override;
    /**
     * @brief isDefaultStreamableObj Get if the default streamable output is in use.
     * @return true if is it use, false if replaced by another one
     */
    bool isDefaultStreamableObj();
    /**
     * @brief writer Same of getStreamableObj
     * @return current streamable output
     */
    std::shared_ptr<Memory::Streams::StreamableObject> writer() { return m_outStream; }
    /**
     * @brief getStreamableObj Get the current streamable output
     * @return current streamable output
     */
    std::shared_ptr<Memory::Streams::StreamableObject> getStreamableObj();
    /**
     * @brief setStreamableObj Set the streamable output (eg. a file?)
     * @param outDataContainer stream that will be used for the content trnasmission
     */
    void setStreamableObj(std::shared_ptr<Memory::Streams::StreamableObject> outDataContainer);
    /**
     * @brief getStreamSize Get stream full size ()
     * @return std::numeric_limits<uint64_t>::max() if size not defined, or >=0 if size defined.
     */
    uint64_t getStreamSize();
    /**
     * @brief postVars Get the post vars (read-only)... Useful for decoding received content
     * @return Variable
     */
    std::shared_ptr<Memory::Abstract::Vars> postVars();
    /**
     * @brief getMultiPartVars Get the MultiPart POST vars (read/write), call only when the ContainerType is CONTENT_TYPE_MIME, otherwise, runtime error will be triggered
     * @return full MIME message
     */
    std::shared_ptr<MIME::MIME_Message> getMultiPartVars();
    /**
     * @brief getUrlPostVars Get URL formatted POST Vars (read/write), call only when the ContainerType is CONTENT_TYPE_URL, otherwise, runtime error will be triggered
     * @return URL Var object
     */
    std::shared_ptr<URLVars> getUrlPostVars();

    //////////////////////////////////////////////////////////////////
    // ------------------ TRANSMISSION AND CONTENT ------------------
    //////////////////////////////////////////////////////////////////
    /**
     * @brief setTransmitionMode Set the transmission mode (chunked, content-lenght, connection-close)
     * @param value mode
     */
    void setTransmitionMode(const eTransmitionMode &value);
    /**
     * @brief getTransmitionMode Get the transmission mode (chunked, content-lenght, connection-close)
     * @return mode
     */
    eTransmitionMode getTransmitionMode() const;
    /**
     * @brief setContentLenSize Set content length data size (for receiving/decoding data).
     * @param contentLengthSize size.
     * @return true if limits are not exceeded.
     */
    bool setContentLenSize(const uint64_t &contentLengthSize);
    /**
     * @brief useFilesystem Set filesystem path when variables exceed limits
     * @param fsFilePath file path
     *
    void useFilesystem(const std::string & fsFilePath);*/
    /**
     * @brief setContainerType Set Container Content Data Type (URL/MIME/BIN)
     * @param value type
     */
    void setContainerType(const eDataType &value);
    /**
     * @brief getContainerType Get container content data type (URL/MIME/BIN)
     * @return type
     */
    eDataType getContainerType() const;


    //////////////////////////////////////////////////
    // Security:
    /**
     * @brief Sets the maximum size of the internal memory buffer used to store HTTP POST data before the data is written to disk instead.
     * @param value The maximum size of the buffer, in bytes.
     */
    void setMaxBinPostMemoryBeforeFS(const uint64_t &value);
    void setSecurityMaxPostDataSize(const uint64_t &value);
    void setSecurityMaxHttpChunkSize(const uint32_t &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    std::shared_ptr<Memory::Streams::StreamableObject> m_outStream = std::make_shared<Memory::Containers::B_Chunks>();
    bool m_usingInternalOutStream = true;

    uint32_t parseHttpChunkSize();

    // Parsing Optimization:
    eTransmitionMode m_transmitionMode = TRANSMIT_MODE_CONNECTION_CLOSE;
    eProcessingMode m_currentMode = PROCMODE_CONTENT_LENGTH;
    eDataType m_containerType = CONTENT_TYPE_BIN;

    // Security Parameters (for parsing):
    uint64_t m_securityMaxPostDataSize = 17*MB_MULT; // 17Mb intermediate buffer (suitable for 16mb max chunk...).
    uint64_t m_currentContentLengthSize = 0;
    uint32_t m_securityMaxHttpChunkSize = 16*MB_MULT; // 16mb.
};

}}}}}

#endif // HTTP_CONTENT_H
