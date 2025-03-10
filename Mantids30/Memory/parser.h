#pragma once

#include "subparser.h"
#include <memory>

namespace Mantids30 { namespace Memory { namespace Streams {


/**
 * @brief The ProtocolParser_Base class
 *
 * Via Object:
 *                                             *------------------*
 * Current Parser <---- Binary <--- Parser <---|    Streamable    |
 *                      Object                 *------------------*
 *
 * Via SO: (you can use writeStream and streamTo)
 *
 * Current Parser <---- Binary <--- Parser (Streamable)
 *                      Object
 *
 *
 */
class Parser : public Memory::Streams::StreamableObject
{
public:
    Parser(std::shared_ptr<Memory::Streams::StreamableObject> value, bool clientMode);
    virtual ~Parser() override = default;
    enum ErrorMSG {
        PARSING_SUCCEED = 0,
        PARSING_ERR_INIT = -1,
        PARSING_ERR_READ = -2,
        PARSING_ERR_PARSE = -3
    };
    /**
     * @brief parseObject Parse streamable object (init/parse/end)
     * @param err: (0:succeed, -1:failed to initialize, -2:failed to read, -3:failed to parse/write)
     * @return bytes written.
     */
    Status parseObject(ErrorMSG * err);

    //////////////////////////////////////////
    virtual bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject>, Status &) override;
    virtual Status write(const void * buf, const size_t &count, Status &wrStat) override;
    /**
     * @brief writeEOF Receive this when the connection is ended.
     */
    virtual void writeEOF(bool) override;

    //////////////////////////////////////////
    /**
     * @brief setMaxTTL Set Max TTL for parsing data
     * @param value max times parseData should be called
     */
    void setMaxTTL(const size_t &value);

    void setStreamable(std::shared_ptr<StreamableObject> value);

protected:
    //////////////////////////////////////////
    // Virtual functions to initialize the protocol.
    virtual bool initProtocol() = 0;
    virtual void endProtocol() = 0;
    virtual bool changeToNextParser() = 0;

    std::shared_ptr<Memory::Streams::StreamableObject> m_streamableObject;

    void initSubParser(SubParser * subparser);

    SubParser * m_currentParser;
    size_t m_maxTTL;
    bool m_initialized;
    bool m_clientMode;

private:
    /**
     * @brief parseData parse data into current parser. This calls recursively
     *                  until all data is filled.
     * @param buf data to be parsed
     * @param count data size to be parsed
     * @param ttl Time To Live Counter.
     * @return -1 if error, and n>0 : n length of data processed by parser, which should be equal to count.
     */
    std::pair<bool,uint64_t> parseData(const void * buf, size_t count, size_t *ttl, bool * finished);
};



}}}



