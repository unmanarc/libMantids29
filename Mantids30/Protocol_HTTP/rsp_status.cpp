#include "rsp_status.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::HTTP;
using namespace Mantids30;

const sHTTP_StatusCode Status::responseRetCodes[] = {
    {100,"Continue"},
    {101,"Switching Protocol"},
    {200,"OK"},
    {201,"Created"},
    {202,"Accepted"},
    {203,"Non-Authoritative Information"},
    {204,"No Content"},
    {205,"Reset Content"},
    {206,"Partial Content"},
    {300,"Multiple Choices"},
    {301,"Moved Permanently"},
    {302,"Found"},
    {303,"See Other"},
    {304,"Not Modified"},
    {307,"Temporary Redirect"},
    {308,"Permanent Redirect"},
    {400,"Bad Request"},
    {401,"Unauthorized"},
    {403,"Forbidden"},
    {404,"Not Found"},
    {405,"Method Not Allowed"},
    {406,"Not Acceptable"},
    {407,"Proxy Authentication Required"},
    {408,"Request Timeout"},
    {409,"Conflict"},
    {410,"Gone"},
    {411,"Length Required"},
    {412,"Precondition Failed"},
    {413,"Payload Too Large"},
    {414,"URI Too Long"},
    {415,"Unsupported Media Type"},
    {416,"Range Not Satisfiable"},
    {417,"Expectation Failed"},
    {426,"Upgrade Required"},
    {428,"Precondition Required"},
    {429,"Too Many Requests"},
    {431,"Request Header Fields Too Large"},
    {451,"Unavailable For Legal Reasons"},
    {500,"Internal Server Error"},
    {501,"Not Implemented"},
    {502,"Bad Gateway"},
    {503,"Service Unavailable"},
    {504,"Gateway Timeout"},
    {505,"HTTP Version Not Supported"},
    {511,"Network Authentication Required"}
};


Status::Status()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setParseDataTargetSize(128);
    subParserName = "Status";

}

uint16_t Status::getHTTPStatusCodeTranslation(const Status::eRetCode &code)
{
    if (code != HTTP::Status::S_999_NOT_SET)
    {
        return responseRetCodes[code].code;
    }
    return 999;
}

Memory::Streams::SubParser::ParseStatus Status::parse()
{
    std::string clientRequest = getParsedBuffer()->toString();

    vector<string> requestParts;
    split(requestParts,clientRequest,is_any_of("\t "),token_compress_on);

    // We need almost 2 parameters.
    if (requestParts.size()<2) return Memory::Streams::SubParser::PARSE_STAT_ERROR;

    httpVersion.parse(requestParts[0]);
    responseRetCode = strtoul(requestParts[1].c_str(),nullptr,10);
    responseMessage = "";

    if (requestParts.size()>=3)
    {
        for (size_t i=2; i<requestParts.size(); i++)
        {
            if (i!=2) responseMessage+=" ";
            responseMessage+=requestParts[i];
        }
    }

    return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
}

std::string Status::getResponseMessage() const
{
    return responseMessage;
}

void Status::setResponseMessage(const std::string &value)
{
    responseMessage = value;
}

bool Status::stream(Memory::Streams::StreamableObject::Status & wrStat)
{
    // Act as a client. Send data from here.
    return upStream->writeString(  httpVersion.toString()
                                 +  " "
                                 +  std::to_string(responseRetCode)
                                 +  " "
                                 +  responseMessage + "\r\n",wrStat).succeed;
}

void Status::setRetCodeValue(unsigned short value)
{
    responseRetCode = value;
}

void Status::setRetCode(Status::eRetCode code)
{
    if (code != HTTP::Status::S_999_NOT_SET)
    {
        setRetCodeValue(responseRetCodes[code].code);
        setResponseMessage(responseRetCodes[code].responseMessage);
    }
}

unsigned short Status::getRetCode() const
{
    return responseRetCode;
}

Common::Version *Status::getHttpVersion()
{
    return &httpVersion;
}


