#pragma once

#include <Mantids30/Memory/subparser.h>
#include <Mantids30/Memory/b_base.h>

#include <string>
#include "common_version.h"
#include "common_urlvars.h"

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Request {


class RequestLine : public Memory::Streams::SubParser
{
public:
    RequestLine();
    ////////////////////////////////////////////////
    // Virtuals:
    /**
     * @brief writeInStream Write in Stream declaration from virtual on ProtocolSubParser_Base
     *        writes this class objects/data into the http uplink.
     * @return true if written successfully/
     */
    bool stream(Memory::Streams::StreamableObject::Status & wrStat) override;

    ////////////////////////////////////////////////
    // Objects:
    /**
     * @brief getHttpVersion Get HTTP Version requested.
     * @return version object requested.
     */
    Common::Version * getHTTPVersion();
    /**
     * @brief getGETVars Get object that handles HTTP Vars
     * @return object that handles http vars.
     */
    std::shared_ptr<Memory::Abstract::Vars> urlVars();

    //////////////////////////////////////////////////
    // Local getters/setters.
    /**
     * @brief Get Request Method (GET/POST/HEAD/...)
     * @return request method string.
     */
    std::string getRequestMethod() const;
    void setRequestMethod(const std::string &value);

    std::string getURI() const;
    void setRequestURI(const std::string &value);

    //////////////////////////////////////////////////
    // Security:
    void setSecurityMaxURLSize(size_t value);

    std::string getRequestURIParameters() const;

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    void parseURI();
    void parseGETParameters();

    /**
     * @brief requestMethod - method requested: GET/POST/HEAD/...
     */
    std::string m_requestMethod = "GET"; // Default Method.
    /**
     * @brief requestURL - URL Requested (without vars) E.g. /index.html
     */
    std::string m_requestURI;
    /**
     * @brief request URI Parameters
     */
    std::string m_requestURIParameters;

    Common::Version m_httpVersion;
    std::shared_ptr<Common::URLVars> m_getVars;
};

}}}}}

