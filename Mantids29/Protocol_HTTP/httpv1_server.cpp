#include "httpv1_server.h"

#include "hdr_cookie.h"

#include <vector>
#include <stdexcept>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <Mantids29/Helpers/encoders.h>
#include <Mantids29/Memory/b_mmap.h>

#include <sys/stat.h>

#ifdef _WIN32
#define SLASHB '\\'
#define SLASH "\\"
#include <boost/algorithm/string/predicate.hpp>
#include <stdlib.h>
// TODO: check if _fullpath mitigate transversal.
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#else
#define SLASH "/"
#define SLASHB '/'
#endif

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace Mantids29::Protocols::HTTP;
using namespace Mantids29;

HTTPv1_Server::HTTPv1_Server(Memory::Streams::StreamableObject *sobject) : HTTPv1_Base(false, sobject)
{
    badAnswer = false;

    // All request will have no-cache activated.... (unless it's a real file and it's not overwritten)
    serverResponse.cacheControl.setOptionNoCache(true);
    serverResponse.cacheControl.setOptionNoStore(true);
    serverResponse.cacheControl.setOptionMustRevalidate(true);

    currentParser = (Memory::Streams::SubParser *)(&clientRequest.requestLine);

    // Default Mime Types (ref: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types)
    m_mimeTypes[".aac"]    = "audio/aac";
    m_mimeTypes[".abw"]    = "application/x-abiword";
    m_mimeTypes[".arc"]    = "application/x-freearc";
    m_mimeTypes[".avi"]    = "video/x-msvideo";
    m_mimeTypes[".azw"]    = "application/vnd.amazon.ebook";
    m_mimeTypes[".bin"]    = "application/octet-stream";
    m_mimeTypes[".bmp"]    = "image/bmp";
    m_mimeTypes[".bz"]     = "application/x-bzip";
    m_mimeTypes[".bz2"]    = "application/x-bzip2";
    m_mimeTypes[".csh"]    = "application/x-csh";
    m_mimeTypes[".css"]    = "text/css";
    m_mimeTypes[".csv"]    = "text/csv";
    m_mimeTypes[".doc"]    = "application/msword";
    m_mimeTypes[".docx"]   = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    m_mimeTypes[".eot"]    = "application/vnd.ms-fontobject";
    m_mimeTypes[".epub"]   = "application/epub+zip";
    m_mimeTypes[".gz"]     = "application/gzip";
    m_mimeTypes[".gif"]    = "image/gif";
    m_mimeTypes[".htm"]    = "text/html";
    m_mimeTypes[".html"]   = "text/html";
    m_mimeTypes[".iso"]    = "application/octet-stream";
    m_mimeTypes[".ico"]    = "image/vnd.microsoft.icon";
    m_mimeTypes[".ics"]    = "text/calendar";
    m_mimeTypes[".jar"]    = "application/java-archive";
    m_mimeTypes[".jpeg"]   = "image/jpeg";
    m_mimeTypes[".jpg"]    = "image/jpeg";
    m_mimeTypes[".js"]     = "application/javascript";
    m_mimeTypes[".json"]   = "application/json";
    m_mimeTypes[".jsonld"] = "application/ld+json";
    m_mimeTypes[".mid"]    = "audio/midi";
    m_mimeTypes[".midi"]   = "audio/x-midi";
    m_mimeTypes[".mjs"]    = "text/javascript";
    m_mimeTypes[".mp3"]    = "audio/mpeg";
    m_mimeTypes[".mp4"]    = "video/mp4";
    m_mimeTypes[".mpeg"]   = "video/mpeg";
    m_mimeTypes[".mpg"]    = "video/mpeg";
    m_mimeTypes[".mpkg"]   = "application/vnd.apple.installer+xml";
    m_mimeTypes[".odp"]    = "application/vnd.oasis.opendocument.presentation";
    m_mimeTypes[".ods"]    = "application/vnd.oasis.opendocument.spreadsheet";
    m_mimeTypes[".odt"]    = "application/vnd.oasis.opendocument.text";
    m_mimeTypes[".oga"]    = "audio/ogg";
    m_mimeTypes[".ogv"]    = "video/ogg";
    m_mimeTypes[".ogx"]    = "application/ogg";
    m_mimeTypes[".opus"]   = "audio/opus";
    m_mimeTypes[".otf"]    = "font/otf";
    m_mimeTypes[".png"]    = "image/png";
    m_mimeTypes[".pdf"]    = "application/pdf";
    m_mimeTypes[".php"]    = "application/x-httpd-php";
    m_mimeTypes[".ppt"]    = "application/vnd.ms-powerpoint";
    m_mimeTypes[".pptx"]   = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    m_mimeTypes[".rar"]    = "application/vnd.rar";
    m_mimeTypes[".rtf"]    = "application/rtf";
    m_mimeTypes[".sh"]     = "application/x-sh";
    m_mimeTypes[".svg"]    = "image/svg+xml";
    m_mimeTypes[".swf"]    = "application/x-shockwave-flash";
    m_mimeTypes[".tar"]    = "application/x-tar";
    m_mimeTypes[".tif"]    = "image/tiff";
    m_mimeTypes[".ts"]     = "video/mp2t";
    m_mimeTypes[".ttf"]    = "font/ttf";
    m_mimeTypes[".txt"]    = "text/plain";
    m_mimeTypes[".vsd"]    = "application/vnd.visio";
    m_mimeTypes[".wav"]    = "audio/wav";
    m_mimeTypes[".weba"]   = "audio/webm";
    m_mimeTypes[".webm"]   = "video/webm";
    m_mimeTypes[".webp"]   = "image/webp";
    m_mimeTypes[".woff"]   = "font/woff";
    m_mimeTypes[".woff2"]  = "font/woff2";
    m_mimeTypes[".xhtml"]  = "application/xhtml+xml";
    m_mimeTypes[".xls"]    = "application/vnd.ms-excel";
    m_mimeTypes[".xlsx"]   = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    m_mimeTypes[".xml"]    = "application/xml";
    m_mimeTypes[".xul"]    = "application/vnd.mozilla.xul+xml";
    m_mimeTypes[".zip"]    = "application/zip";
    m_mimeTypes[".3gp"]    = "video/3gpp";
    m_mimeTypes[".3g2"]    = "video/3gpp2";
    m_mimeTypes[".7z"]     = "application/x-7z-compressed";

    m_includeServerDate = true;
}

void HTTPv1_Server::fillRequestDataStruct()
{
    clientVars.VARS_GET = clientRequest.getVars(HTTP_VARS_GET);
    clientVars.VARS_POST = clientRequest.getVars(HTTP_VARS_POST);
    clientVars.VARS_COOKIES = clientRequest.headers.getOptionByName("Cookie");
}

void HTTPv1_Server::setRemotePairAddress(const char *value)
{
    SecBACopy(clientVars.REMOTE_ADDR,value);
}


void HTTPv1_Server::setResponseServerName(const string &sServerName)
{
    serverResponse.headers.replace("Server", sServerName);
}

bool HTTPv1_Server::getLocalFilePathFromURI2(string sServerDir, sLocalRequestedFileInfo *info, const std::string &defaultFileAppend, const bool &dontMapExecutables)
{
    if (!info)
        throw std::runtime_error( std::string(__func__) + std::string(" Should be called with info object... Aborting...") );

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir=realpath((sServerDir).c_str(), nullptr))==nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath =    sServerDir           // Put the current server dir...
            + (clientRequest.getURI().empty()?"":clientRequest.getURI().substr(1)) // Put the Request URI (without the first character / slash)
            + defaultFileAppend; // Append option...

    struct stat stats;

    // Compute the full requested path:
    std::string sFullComputedPath;
    {
        char *cFullPath;
        if (  m_staticContentElements.find(std::string(clientRequest.getURI()+defaultFileAppend)) != m_staticContentElements.end() )
        {
            // STATIC CONTENT:
            serverResponse.cacheControl.setOptionNoCache(false);
            serverResponse.cacheControl.setOptionNoStore(false);
            serverResponse.cacheControl.setOptionMustRevalidate(false);
            serverResponse.cacheControl.setMaxAge(3600);
            serverResponse.cacheControl.setOptionImmutable(true);

            info->sRealRelativePath = clientRequest.getURI()+defaultFileAppend;

            setResponseContentTypeByFileExtension(info->sRealRelativePath);

            info->sRealFullPath = "MEM:" + info->sRealRelativePath;

            serverResponse.setDataStreamer(m_staticContentElements[info->sRealRelativePath],false);
            return true;
        }
        else if ((cFullPath=realpath(sFullRequestedPath.c_str(), nullptr))!=nullptr)
        {
            // Compute the full path..
            sFullComputedPath = cFullPath;
            free(cFullPath);

            // Check file properties...
            stat(sFullComputedPath.c_str(), &stats);

            // Put a slash at the end of the computed dir resource (when dir)...
            if ((info->isDir = S_ISDIR(stats.st_mode)) == true)
                sFullComputedPath += (sFullComputedPath.back() == SLASHB ? "" : std::string(SLASH));

            // Path OK, continue.
        }
        else
        {
            // Does not exist or unaccesible. (404)
            return false;
        }
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedPath.size()<sServerDir.size() || memcmp(sServerDir.c_str(),sFullComputedPath.c_str(),sServerDir.size())!=0)
    {
        info->isTransversal=true;
        return false;
    }

    // No transversal detected at this point.

    // Check if it's a directory...

    if (info->isDir == true)
    {
        info->pathExist=true;

        // Don't get directories when we are appending something.
        if (!defaultFileAppend.empty())
            return false;

        // Complete the directory notation (slash at the end)
        if (sFullComputedPath.back()!=SLASHB)
            sFullComputedPath += SLASH;

        info->sRealFullPath = sFullComputedPath;
        info->sRealRelativePath = sFullComputedPath.c_str()+(sServerDir.size()-1);

        if (!access(sFullComputedPath.c_str(),R_OK))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( S_ISREG(stats.st_mode) == true  ) // Check if it's a regular file
    {
        info->pathExist=true;

        if (    dontMapExecutables &&
        #ifndef _WIN32
                !access(sFullComputedPath.c_str(),X_OK)
        #else
                (boost::iends_with(sFullComputedPath,".exe") || boost::iends_with(sFullComputedPath,".bat") || boost::iends_with(sFullComputedPath,".com"))
        #endif
                )
        {
            // file is executable... don't map, and the most important: don't create cache in the browser...
            // Very useful for CGI-like implementations...
            info->sRealFullPath = sFullComputedPath;
            info->sRealRelativePath = sFullComputedPath.c_str()+(sServerDir.size()-1);
            info->isExecutable = true;
            info->pathExist = true;
            return true;
        }
        else
        {
            Mantids29::Memory::Containers::B_MMAP * bFile = new Mantids29::Memory::Containers::B_MMAP;
            if (bFile->referenceFile(sFullComputedPath.c_str(),true,false))
            {
                // File Found / Readable.
                info->sRealFullPath = sFullComputedPath;
                info->sRealRelativePath = sFullComputedPath.c_str()+(sServerDir.size()-1);
                serverResponse.setDataStreamer(bFile,true);
                setResponseContentTypeByFileExtension(info->sRealRelativePath);

                struct stat attrib;
                if (!stat(sFullRequestedPath.c_str(), &attrib))
                {
                    HTTP::Common::Date fileModificationDate;
#ifdef _WIN32
                    fileModificationDate.setUnixTime(attrib.st_mtime);
#else
                    fileModificationDate.setUnixTime(attrib.st_mtim.tv_sec);
#endif
                    if (m_includeServerDate)
                        serverResponse.headers.add("Last-Modified", fileModificationDate.toString());
                }

                serverResponse.cacheControl.setOptionNoCache(false);
                serverResponse.cacheControl.setOptionNoStore(false);
                serverResponse.cacheControl.setOptionMustRevalidate(false);
                serverResponse.cacheControl.setMaxAge(3600);
                serverResponse.cacheControl.setOptionImmutable(true);
                return true;
            }
            else
            {
                // File not found / Readable...
                delete bFile;
                return false;
            }
        }
    }
    else
    {
        // Special files...
        return false;
    }
}

bool HTTPv1_Server::getLocalFilePathFromURI0NE(const std::string & uri, std::string sServerDir, sLocalRequestedFileInfo *info)
{
    if (!info)
        throw std::runtime_error( std::string(__func__) + std::string(" Should be called with info object... Aborting...") );

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir=realpath((sServerDir).c_str(), nullptr))==nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath =    sServerDir           // Put the current server dir...
            + (uri.empty()?"":uri.substr(1)) // Put the Request URI (without the first character / slash)
            ; // Append option...

    // Compute the full requested path:
    std::string sFullComputedRealPath;
    {
        char cRealPath[PATH_MAX+2];
        realpath(sFullRequestedPath.c_str(), cRealPath);
        if ( errno == ENOENT )
        {
            // Non-Existant File.
            sFullComputedRealPath = cRealPath;
        }
        else
            return false;
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedRealPath.size()<sServerDir.size() || memcmp(sServerDir.c_str(),sFullComputedRealPath.c_str(),sServerDir.size())!=0)
    {
        info->isTransversal=true;
        return false;
    }

    // No transversal detected at this point.
    info->sRealFullPath = sFullComputedRealPath;
    info->sRealRelativePath = sFullComputedRealPath.c_str()+(sServerDir.size()-1);

    return true;
}

bool HTTPv1_Server::getLocalFilePathFromURI0E(const std::string &uri, std::string sServerDir, sLocalRequestedFileInfo *info)
{
    if (!info)
        throw std::runtime_error( std::string(__func__) + std::string(" Should be called with info object... Aborting...") );

    info->reset();

    {
        char *cServerDir;
        // Check Server Dir Real Path:
        if ((cServerDir=realpath((sServerDir).c_str(), nullptr))==nullptr)
            return false;

        sServerDir = cServerDir;

        // Put a slash at the end of the server dir resource...
        sServerDir += (sServerDir.back() == SLASHB ? "" : std::string(SLASH));

        free(cServerDir);
    }

    // Compute the requested path:
    string sFullRequestedPath =    sServerDir           // Put the current server dir...
            + (uri.empty()?"":uri.substr(1)) // Put the Request URI (without the first character / slash)
            ; // Append option...

    struct stat stats;

    // Compute the full requested path:
    std::string sFullComputedRealPath;
    {
        char cRealPath[PATH_MAX+2];
        if ( realpath(sFullRequestedPath.c_str(), cRealPath) )
        {
            sFullComputedRealPath = cRealPath;

            // Check file properties...
            stat(sFullComputedRealPath.c_str(), &stats);
            // Put a slash at the end of the computed dir resource (when dir)...
            if ((info->isDir = S_ISDIR(stats.st_mode)) == true)
                sFullComputedRealPath += (sFullComputedRealPath.back() == SLASHB ? "" : std::string(SLASH));
        }
        else // Non-Existant File.
            return false;
    }

    // Check for transversal access hacking attempts...
    if (sFullComputedRealPath.size()<sServerDir.size() || memcmp(sServerDir.c_str(),sFullComputedRealPath.c_str(),sServerDir.size())!=0)
    {
        info->isTransversal=true;
        return false;
    }

    // No transversal detected at this point.
    info->sRealFullPath = sFullComputedRealPath;
    info->sRealRelativePath = sFullComputedRealPath.c_str()+(sServerDir.size()-1);

    return true;
}

bool HTTPv1_Server::setResponseContentTypeByFileExtension(const string &sFilePath)
{
    const char * cFileExtension = strrchr(sFilePath.c_str(),'.');

    if (!cFileExtension || cFileExtension[1]==0)
        return false;

    m_currentFileExtension = boost::to_lower_copy(std::string(cFileExtension));

    if (m_mimeTypes.find(m_currentFileExtension) != m_mimeTypes.end())
    {
        serverResponse.setContentType(m_mimeTypes[m_currentFileExtension],true);
        return true;
    }
    serverResponse.setContentType("",false);
    return false;
}

void HTTPv1_Server::addResponseContentTypeFileExtension(const string &ext, const string &type)
{
    m_mimeTypes[ext] = type;
}

bool HTTPv1_Server::changeToNextParser()
{
    // Server Mode:
    if (currentParser == &clientRequest.requestLine) return changeToNextParserOnClientRequest();
    else if (currentParser == &clientRequest.headers) return changeToNextParserOnClientHeaders();
    else return changeToNextParserOnClientContentData();
}

bool HTTPv1_Server::changeToNextParserOnClientHeaders()
{
    // This function is used when the client options arrives, so we need to parse it.

    // Internal checks when options are parsed (ex. check if host exist on http/1.1)
    parseHostOptions();
    prepareServerVersionOnOptions();

    // PARSE CLIENT BASIC AUTHENTICATION:
    clientRequest.basicAuth.bEnabled = false;
    if (clientRequest.headers.exist("Authorization"))
    {
        vector<string> authParts;
        string f1 = clientRequest.headers.getOptionValueStringByName("Authorization");
        split(authParts,f1,is_any_of(" "),token_compress_on);
        if (authParts.size()==2)
        {
            if (authParts[0] == "Basic")
            {
                auto bp = Helpers::Encoders::decodeFromBase64(authParts[1]);
                std::string::size_type pos = bp.find(':', 0);
                if (pos != std::string::npos)
                {
                    clientRequest.basicAuth.bEnabled = true;
                    clientRequest.basicAuth.user = bp.substr(0,pos);
                    clientRequest.basicAuth.pass = bp.substr(pos+1,bp.size());
                }

            }
        }
    }

    // PARSE USER-AGENT
    if (clientRequest.headers.exist("User-Agent"))
        clientRequest.userAgent = clientRequest.headers.getOptionRawStringByName("User-Agent");

    // PARSE CONTENT TYPE/LENGHT OPTIONS
    if (badAnswer)
        return answer(m_answerBytes);
    else
    {
        uint64_t contentLength = clientRequest.headers.getOptionAsUINT64("Content-Length");
        string contentType = clientRequest.headers.getOptionValueStringByName("Content-Type");
        /////////////////////////////////////////////////////////////////////////////////////
        // Content-Length...
        if (contentLength)
        {
            // Content length defined.
            clientRequest.content.setTransmitionMode(Common::Content::TRANSMIT_MODE_CONTENT_LENGTH);
            if (!clientRequest.content.setContentLenSize(contentLength))
            {
                // Error setting this content length size. (automatic answer)
                badAnswer = true;
                serverResponse.status.setRetCode(HTTP::Status::S_413_PAYLOAD_TOO_LARGE);
                return answer(m_answerBytes);
            }
            /////////////////////////////////////////////////////////////////////////////////////
            // Content-Type... (only if length is designated)
            if ( icontains(contentType,"multipart/form-data") )
            {
                clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_MIME);
                clientRequest.content.getMultiPartVars()->setMultiPartBoundary(clientRequest.headers.getOptionByName("Content-Type")->getSubVar("boundary"));
            }
            else if ( icontains(contentType,"application/x-www-form-urlencoded") )
            {
                clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_URL);
            }
            else
                clientRequest.content.setContainerType(Common::Content::CONTENT_TYPE_BIN);
            /////////////////////////////////////////////////////////////////////////////////////
        }

        // Process the client header options
        if (!badAnswer)
        {
            fillRequestDataStruct();

            if (!procHTTPClientHeaders())
                currentParser = nullptr; // Don't continue with parsing (close the connection)
            else
            {
                // OK, we are ready.
                if (contentLength) currentParser = &clientRequest.content;
                else
                {
                    // Answer here:
                    return answer(m_answerBytes);
                }
            }
        }
    }
    return true;
}

bool HTTPv1_Server::changeToNextParserOnClientRequest()
{
    // Internal checks when URL request has received.
    prepareServerVersionOnURI();
    if (badAnswer)
        return answer(m_answerBytes);
    else
    {
        fillRequestDataStruct();

        if (!procHTTPClientURI())
            currentParser = nullptr; // Don't continue with parsing.
        else currentParser = &clientRequest.headers;
    }
    return true;
}

bool HTTPv1_Server::changeToNextParserOnClientContentData()
{
    return answer(m_answerBytes);
}

bool HTTPv1_Server::streamServerHeaders(Memory::Streams::StreamableObject::Status &wrStat)
{
    // Act as a server. Send data from here.
    uint64_t strsize;

    if ((strsize=serverResponse.content.getStreamSize()) == std::numeric_limits<uint64_t>::max())
    {
        // TODO: connection keep alive.
        serverResponse.headers.add("Connetion", "Close");
        serverResponse.headers.remove("Content-Length");
        /////////////////////
        if (serverResponse.content.getTransmitionMode() == Common::Content::TRANSMIT_MODE_CHUNKS)
            serverResponse.headers.replace("Transfer-Encoding", "Chunked");
    }
    else
    {
        serverResponse.headers.remove("Connetion");
        serverResponse.headers.replace("Content-Length", std::to_string(strsize));
    }

    HTTP::Common::Date currentDate;
    currentDate.setCurrentTime();
    if (m_includeServerDate)
        serverResponse.headers.add("Date", currentDate.toString());

    if (!serverResponse.sWWWAuthenticateRealm.empty())
    {
        serverResponse.headers.add("WWW-Authenticate", "Basic realm=\""+ serverResponse.sWWWAuthenticateRealm + "\"");
    }

    // Establish the cookies
    serverResponse.headers.remove("Set-Cookie");
    serverResponse.cookies.putOnHeaders(&serverResponse.headers);

    // Security Options...
    serverResponse.headers.replace("X-XSS-Protection", serverResponse.security.XSSProtection.toString());

    std::string cacheOptions = serverResponse.cacheControl.toString();
    if (!cacheOptions.empty())
        serverResponse.headers.replace("Cache-Control", cacheOptions);

    if (!serverResponse.security.XFrameOpts.isNotActivated())
        serverResponse.headers.replace("X-Frame-Options", serverResponse.security.XFrameOpts.toString());

    // TODO: check if this is a secure connection.. (Over TLS?)
    if (serverResponse.security.HSTS.m_activated)
        serverResponse.headers.replace("Strict-Transport-Security", serverResponse.security.HSTS.toString());

    // Content Type...
    if (!serverResponse.contentType.empty())
    {
        serverResponse.headers.replace("Content-Type", serverResponse.contentType);
        if (serverResponse.security.bNoSniffContentType)
            serverResponse.headers.replace("X-Content-Type-Options", "nosniff");
    }

    return serverResponse.headers.stream(wrStat);
}

void HTTPv1_Server::prepareServerVersionOnURI()
{
    serverResponse.status.getHttpVersion()->setMajor(1);
    serverResponse.status.getHttpVersion()->setMinor(0);

    if (clientRequest.requestLine.getHTTPVersion()->getMajor()!=1)
    {
        serverResponse.status.setRetCode(HTTP::Status::S_505_HTTP_VERSION_NOT_SUPPORTED);
        badAnswer = true;
    }
    else
    {
        serverResponse.status.getHttpVersion()->setMinor(clientRequest.requestLine.getHTTPVersion()->getMinor());
    }
}

void HTTPv1_Server::prepareServerVersionOnOptions()
{
    if (clientRequest.requestLine.getHTTPVersion()->getMinor()>=1)
    {
        if (clientRequest.virtualHost=="")
        {
            // TODO: does really need the VHost?
            serverResponse.status.setRetCode(HTTP::Status::S_400_BAD_REQUEST);
            badAnswer = true;
        }
    }
}

void HTTPv1_Server::parseHostOptions()
{
    string hostVal = clientRequest.headers.getOptionValueStringByName("HOST");
    if (!hostVal.empty())
    {
        clientRequest.virtualPort = 80;
        vector<string> hostParts;
        split(hostParts,hostVal,is_any_of(":"),token_compress_on);
        if (hostParts.size()==1)
        {
            clientRequest.virtualHost = hostParts[0];
        }
        else if (hostParts.size()>1)
        {
            clientRequest.virtualHost = hostParts[0];
            clientRequest.virtualPort = (uint16_t)strtoul(hostParts[1].c_str(),nullptr,10);
        }
    }
}


bool HTTPv1_Server::answer(Memory::Streams::StreamableObject::Status &wrStat)
{
    wrStat.bytesWritten = 0;

#ifndef WIN32
    pthread_setname_np(pthread_self(), "HTTP:Response");
#endif

    fillRequestDataStruct();

    // Process client petition here.
    if (!badAnswer) serverResponse.status.setRetCode(procHTTPClientContent());

    // Answer is the last... close the connection after it.
    currentParser = nullptr;

    if (!serverResponse.status.stream(wrStat))
        return false;
    if (!streamServerHeaders(wrStat))
        return false;
    if (!serverResponse.content.stream(wrStat))
    {
        serverResponse.content.preemptiveDestroyStreamableObj();
        return false;
    }

    // If all the data was sent OK, ret true, and destroy the external container.
    serverResponse.content.preemptiveDestroyStreamableObj();
    return true;
}

void HTTPv1_Server::setStaticContentElements(const std::map<std::string, Mantids29::Memory::Containers::B_MEM *> &value)
{
    m_staticContentElements = value;
}

std::string HTTPv1_Server::htmlEncode(const std::string &rawStr)
{
    std::string output;
    output.reserve(rawStr.size());
    for(size_t i=0; rawStr.size()!=i; i++)
    {
        switch(rawStr[i])
        {
        case '<':
            output.append("&lt;");
            break;
        case '>':
            output.append("&gt;");
            break;
        case '\"':
            output.append("&quot;");
            break;
        case '&':
            output.append("&amp;");
            break;
        case '\'':
            output.append("&apos;");
            break;
        default:
            output.append(&rawStr[i], 1);
            break;
        }
    }
    return output;
}

bool HTTPv1_Server::verifyStaticContentExistence(const string &path)
{
    return !(m_staticContentElements.find(path) == m_staticContentElements.end());
}


std::string HTTPv1_Server::getCurrentFileExtension() const
{
    return m_currentFileExtension;
}

void HTTPv1_Server::setResponseIncludeServerDate(bool value)
{
    m_includeServerDate = value;
}

bool HTTPv1_Server::isSecure() const
{
    return m_isSecure;
}

void HTTPv1_Server::setSecure(bool value)
{
    m_isSecure = value;
}

void HTTPv1_Server::addStaticContent(const string &path, Memory::Containers::B_MEM *contentElement)
{
    m_staticContentElements[path] = contentElement;
}

Memory::Streams::StreamableObject::Status HTTPv1_Server::getResponseTransmissionStatus() const
{
    return m_answerBytes;
}

Memory::Streams::StreamableObject::Status HTTPv1_Server::streamResponse(Memory::Streams::StreamableObject *source)
{
    Memory::Streams::StreamableObject::Status stat;

    if (!serverResponse.content.getStreamableObj())
    {
        stat.succeed = false;
        return stat;
    }

    source->streamTo( serverResponse.content.getStreamableObj(), stat);
    return stat;
}

Protocols::HTTP::Status::eRetCode HTTPv1_Server::setResponseRedirect(const string &location, bool temporary)
{
    serverResponse.headers.replace("Location", location);

    // Remove previous data streamer:
    serverResponse.setDataStreamer(nullptr);

    if (temporary)
        return HTTP::Status::S_307_TEMPORARY_REDIRECT;
    else
        return HTTP::Status::S_308_PERMANENT_REDIRECT;
}

Memory::Streams::StreamableObject *HTTPv1_Server::getResponseDataStreamer()
{
    return serverResponse.content.getStreamableObj();
}

