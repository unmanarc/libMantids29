#pragma once

#include <Mantids30/Memory/subparser.h>
#include "common_version.h"
#include <map>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP {

class Status : public Memory::Streams::SubParser
{
public:
    enum Codes {
        S_100_CONTINUE = 100,
        S_101_SWITCHING_PROTOCOLS = 101,
        S_200_OK = 200,
        S_201_CREATED = 201,
        S_202_ACCEPTED = 202,
        S_203_NON_AUTHORITATIVE_INFORMATION = 203,
        S_204_NO_CONTENT = 204,
        S_205_RESET_CONTENT = 205,
        S_206_PARTIAL_CONTENT = 206,
        S_300_MULTIPLE_CHOICES = 300,
        S_301_MOVED_PERMANENTLY = 301,
        S_302_FOUND = 302,
        S_303_SEE_OTHER = 303,
        S_304_NOT_MODIFIED = 304,
        S_307_TEMPORARY_REDIRECT = 307,
        S_308_PERMANENT_REDIRECT = 308,
        S_400_BAD_REQUEST = 400,
        S_401_UNAUTHORIZED = 401,
        S_403_FORBIDDEN = 403,
        S_404_NOT_FOUND = 404,
        S_405_METHOD_NOT_ALLOWED = 405,
        S_406_NOT_ACCEPTABLE = 406,
        S_407_PROXY_AUTHENTICATION_REQUIRED = 407,
        S_408_REQUEST_TIMEOUT = 408,
        S_409_CONFLICT = 409,
        S_410_GONE = 410,
        S_411_LENGTH_REQUIRED = 411,
        S_412_PRECONDITION_FAILED = 412,
        S_413_PAYLOAD_TOO_LARGE = 413,
        S_414_URI_TOO_LONG = 414,
        S_415_UNSUPPORTED_MEDIA_TYPE = 415,
        S_416_RANGE_NOT_SATISFIABLE = 416,
        S_417_EXPECTATION_FAILED = 417,
        S_426_UPGRADE_REQUIRED = 426,
        S_428_PRECONDITION_REQUIRED = 428,
        S_429_TOO_MANY_REQUESTS = 429,
        S_431_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
        S_451_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
        S_500_INTERNAL_SERVER_ERROR = 500,
        S_501_NOT_IMPLEMENTED = 501,
        S_502_BAD_GATEWAY = 502,
        S_503_SERVICE_UNAVAILABLE = 503,
        S_504_GATEWAY_TIMEOUT = 504,
        S_505_HTTP_VERSION_NOT_SUPPORTED = 505,
        S_511_NETWORK_AUTHENTICATION_REQUIRED = 511,
        S_999_NOT_SET = 1000
    };

    Status();

    /**
     * @brief getHTTPVersion - Get HTTP Version Object
     * @return Version Object
     */
    HTTP::Version * getHTTPVersion();
    /**
     * @brief getResponsStatusCodes - Get HTTP Response Code (Ex. 404=Not found)
     * @return response code number
     */
    unsigned short getCode() const;
    /**
     * @brief Set response code and message from a fixed list.
     */
    void setCode(Codes code);
    /**
     * @brief getMessage - Get HTTP Response Code Message (Ex. Not found)
     * @return response code message
     */
    std::string getMessage() const;
    /**
     * @brief setResponseMessage - Set HTTP Response Code Message (Ex. Not found)
     * @param value response code message
     */
    void setResponseMessage(const std::string &value);

    /**
     * @brief Get String Message for specific HTTP Code
     * @param code HTTP Code (eg. 404)
     * @return String
     */
    static std::string getStringTranslation(unsigned short code);

    bool streamToUpstream( ) override;
protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    HTTP::Version m_httpVersion;
    unsigned short m_statusCode = S_999_NOT_SET;
    std::string m_statusMessage;

    static std::map<unsigned short, std::string> m_responseStatusCodesTable;


};
}}}}

