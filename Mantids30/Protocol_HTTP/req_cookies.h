#pragma once

#include <string>
#include <map>

#include <Mantids30/Protocol_MIME/mime_sub_header.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace HTTP { namespace Request {

class Cookies_ClientSide
{
public:
    Cookies_ClientSide();

    void putOnHeaders(MIME::MIME_Sub_Header * headers) const;
    /**
     * @brief parseFromHeaders Parse cookies from string
     * @param cookies_str string to parse
     */
    void parseFromHeaders(const std::string & cookies_str);
    ///////////////////////////

    std::string getCookieByName(const std::string & cookieName);
    void addCookieVal(const std::string & cookieName, const std::string & cookieValue);

    // TODO: add from server cookies (for client use)

private:
    /**
     * @brief toString Convert the cookie to string to send on headers.
     * @return string with cookies.
     */
    std::string toString() const;

    void parseCookie(std::string cookie);
    std::map<std::string,std::string> m_cookiesMap;
};

}}}}}

