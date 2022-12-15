#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include "mdz_auth/data.h"
#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <mdz_xrpc_common/streamablejson.h>
#include <mdz_xrpc_common/methodsmanager.h>
#include <mdz_auth/domains.h>
#include <mdz_auth/multi.h>
#include <mdz_proto_http/httpv1_server.h>

#include <mdz_prg_logs/rpclog.h>
#include <mutex>

namespace Mantids { namespace RPC { namespace Web {

class WebClientHandler : public Protocols::HTTP::HTTPv1_Server
{
public:
    WebClientHandler(void *parent, Memory::Streams::StreamableObject *sock);
    ~WebClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Mantids::Authentication::Domains * authenticator);
    void setMethodsManager(MethodsManager *value);
    //////////////////////////////////////////////

    void setUserIP(const std::string &value);
    void setSessionsManagger(SessionsManager *value);
    void setUseFormattedJSONOutput(bool value);
    void setResourceFilter(ResourcesFilter *value);
    void setDocumentRootPath(const std::string &value);
    void setUsingCSRFToken(bool value);
    void setUseHTMLIEngine(bool value);
    void setRedirectOn404(const std::string &newRedirectOn404);

    void setWebServerName(const std::string &value);
    void setSoftwareVersion(const std::string &value);


    std::string getAppName() const;
    void setAppName(const std::string &value);
    void setRemoteTLSCN(const std::string &value);

    void setRPCLog(Application::Logs::RPCLog *value);


protected:
    /**
     * @brief procHTTPClientContent Process web client request
     * @return http response code.
     */
    Protocols::HTTP::Status::eRetCode procHTTPClientContent() override;
private:
    void sessionOpen();
    void sessionRelease();
    void sessionDestroy();

    Protocols::HTTP::Status::eRetCode procResource_File(Authentication::Multi *extraAuths);
    Protocols::HTTP::Status::eRetCode procResource_HTMLIEngine(const std::string &sRealFullPath, Authentication::Multi *extraAuths);

    Protocols::HTTP::Status::eRetCode procJAPI_Session();
    Protocols::HTTP::Status::eRetCode procJAPI_Session_AUTHINFO();
    Protocols::HTTP::Status::eRetCode procJAPI_Session_CSRFTOKEN();
    Protocols::HTTP::Status::eRetCode procJAPI_Session_LOGIN(const Authentication::Data & auth);
    Protocols::HTTP::Status::eRetCode procJAPI_Session_POSTLOGIN(const Authentication::Data & auth);
    Protocols::HTTP::Status::eRetCode procJAPI_Session_CHPASSWD(const Authentication::Data &auth);
    Protocols::HTTP::Status::eRetCode procJAPI_Session_TESTPASSWD(const Authentication::Data &auth);
    Protocols::HTTP::Status::eRetCode procJAPI_Session_PASSWDLIST();

    Protocols::HTTP::Status::eRetCode procJAPI_Exec( Authentication::Multi *extraAuths,
                                                       std::string sMethodName,
                                                       std::string sPayloadIn,
                                                       Memory::Streams::StreamableJSON * jPayloadOutStr = nullptr
                                                       );
    bool csrfValidate();

    Protocols::HTTP::Status::eRetCode procJAPI_Version();


    std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication::Data &authData, Mantids::Authentication::Session *session, Mantids::Authentication::Reason *authReason);
    Mantids::Authentication::Reason temporaryAuthentication(const std::string &userName, const std::string &domainName, const Authentication::Data &authData);

    //std::string getAuthSessionID(Mantids::Authentication::Session *authSession);

    void log(Mantids::Application::Logs::eLogLevels logSeverity,  const std::string &module, const uint32_t &outSize, const char *fmtLog,... );

    Application::Logs::RPCLog * rpcLog;

    MethodsManager * methodsManager;
    Mantids::Authentication::Domains * authDomains;
    SessionsManager * sessionsManager;

    // Current Session Vars:
    WebSession * webSession;
    Mantids::Authentication::Session *authSession;
    uint64_t uSessionMaxAge;
    std::string sSessionId;
    bool bDestroySession;
    bool bReleaseSessionHandler;
    Authentication::Multi extraCredentials;
    Authentication::Data credentials;

    // Current User Security Vars:
    std::string sClientCSRFToken;



    ResourcesFilter * resourceFilter;
    std::string appName;
    std::string userIP, userTLSCommonName;
    std::string resourcesLocalPath;
    std::string redirectOn404;
    bool useFormattedJSONOutput, usingCSRFToken, useHTMLIEngine;
    std::string webServerName;
    std::string softwareVersion;
};

}}}

#endif // XRPC_SERVER_H
