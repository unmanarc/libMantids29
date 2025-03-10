#include "parser.h"
#include <memory>


using namespace Mantids30::Memory::Streams;

Parser::Parser(std::shared_ptr<StreamableObject> value, bool clientMode)
{
    this->m_clientMode = clientMode;
    m_currentParser = nullptr;
    m_maxTTL = 4096;
    m_initialized = false;
    this->m_streamableObject = value;
}

StreamableObject::Status Parser::parseObject(Parser::ErrorMSG *err)
{
    bool ret;
    Status upd;

    *err = PARSING_SUCCEED;
    m_initialized = initProtocol();
    if (m_initialized)
    {
        if (!(ret=m_streamableObject->streamTo(shared_from_this(),upd)) || !upd.succeed)
        {
            upd.succeed=false;
            *err=getFailedWriteState()!=0?PARSING_ERR_READ:PARSING_ERR_PARSE;
        }
        endProtocol();
        return upd;
    }
    else
        upd.succeed=false;

    *err = PARSING_ERR_INIT;
    return upd;
}

bool Parser::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> , Status &)
{
    return false;
}

StreamableObject::Status Parser::write(const void *buf, const size_t &count, Status &wrStat)
{
    Status ret;
    // Parse this data...
    size_t ttl = 0;
    bool finished = false;

#ifdef DEBUG_PARSER
    printf("Writting on Parser %p:\n", this); fflush(stdout);
    BIO_dump_fp (stdout, static_cast<char *>(buf), count);
    fflush(stdout);
#endif

    std::pair<bool, uint64_t> r = parseData(buf,count, &ttl, &finished);
    if (finished) ret.finish = wrStat.finish = true;

    if (r.first==false)
    {
        wrStat.succeed = ret.succeed = setFailedWriteState();
    }
    else
    {
        ret+=r.second;
        wrStat+=ret;
    }

    return ret;
}

void Parser::writeEOF(bool)
{
    size_t ttl = 0;
    bool finished = false;
    parseData("",0, &ttl,&finished);
}

std::pair<bool, uint64_t> Parser::parseData(const void *buf, size_t count, size_t *ttl, bool *finished)
{
    if (*ttl>m_maxTTL)
    {
        // TODO: reset TTL?
#ifdef DEBUG_PARSER
        fprintf(stderr, "%p Parser reaching TTL %zu", this, *ttl); fflush(stderr);
#endif
        return std::make_pair(false,static_cast<uint64_t>(0)); // TTL Reached...
    }
    (*ttl)++;

    // We are parsing data here...
    // written bytes will be filled with first=error, and second=displacebytes
    // displace bytes is the number of bytes that the subparser have taken from the incoming buffer, so we have to displace them.
    std::pair<bool, uint64_t> writtenBytes;

    if (m_currentParser!=nullptr)
    {
        // Default state: get more data...
        m_currentParser->setParseStatus(SubParser::PARSE_STAT_GET_MORE_DATA);
        // Here, the parser should call the sub stream parser parse function and set the new status.
        if ((writtenBytes=m_currentParser->writeIntoParser(buf,count)).first==false)
            return std::make_pair(false,static_cast<uint64_t>(0));
        // TODO: what if error? how to tell the parser that it should analize the connection up to there (without correctness).
        switch (m_currentParser->getParseStatus())
        {
        case SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER:
        {
#ifdef DEBUG_PARSER
            printf("%p PARSE_STAT_GOTO_NEXT_SUBPARSER requested from %s\n", this, (!m_currentParser?"nullptr" : m_currentParser->getSubParserName().c_str())); fflush(stdout);
#endif
            // Check if there is next parser...
            if (!changeToNextParser())
                return std::make_pair(false,static_cast<uint64_t>(0));
#ifdef DEBUG_PARSER
            printf("%p PARSE_STAT_GOTO_NEXT_SUBPARSER changed to %s\n", this,(!m_currentParser?"nullptr" : m_currentParser->getSubParserName().c_str())); fflush(stdout);
#endif
            // If the parser is changed to nullptr, then the connection is ended (-2).
            // Parsed OK :)... Pass to the next stage
            if (m_currentParser==nullptr)
                *finished = true;
            if (m_currentParser==nullptr || writtenBytes.second == count)
                return writtenBytes;
        } break;
        case SubParser::PARSE_STAT_GET_MORE_DATA:
        {
#ifdef DEBUG_PARSER
            printf("%p PARSE_STAT_GET_MORE_DATA requested from %s\n", this,(!m_currentParser?"nullptr" : m_currentParser->getSubParserName().c_str())); fflush(stdout);
#endif
            // More data required... (TODO: check this)
            if (writtenBytes.second == count)
                return writtenBytes;
        } break;
            // Bad parsing... end here.
        case SubParser::PARSE_STAT_ERROR:
#ifdef DEBUG_PARSER
            printf("%p PARSE_STAT_ERROR executed from %s\n", this,(!m_currentParser?"nullptr" : m_currentParser->getSubParserName().c_str())); fflush(stdout);
#endif

            return std::make_pair(false,static_cast<uint64_t>(0));
            // Unknown parser...
        }
    }

    // TODO: what if writtenBytes == 0?
    // Data left in buffer, process it.
    if (writtenBytes.second!=count)
    {
        buf=(static_cast<const char *>(buf))+writtenBytes.second;
        count-=writtenBytes.second;

        // Data left to process..
        std::pair<bool, uint64_t> x;
        if ((x=parseData(buf,count, ttl, finished)).first==false)
            return x;

        return std::make_pair(true,x.second+writtenBytes.second);
    }
    else
        return writtenBytes;
}

void Parser::initSubParser(SubParser *subparser)
{
    subparser->initElemParser((m_streamableObject!=nullptr)?m_streamableObject:shared_from_this(),m_clientMode);
}

void Parser::setMaxTTL(const size_t &value)
{
    m_maxTTL = value;
}

void Parser::setStreamable(std::shared_ptr<Memory::Streams::StreamableObject> value)
{
    m_streamableObject = value;
}

