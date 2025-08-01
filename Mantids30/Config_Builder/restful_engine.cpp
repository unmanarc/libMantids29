#include "restful_engine.h"

#include "jwt.h"
#include "apiproxyparameters.h"

#include <Mantids30/Net_Sockets/socket_tcp.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <memory>
#include <inttypes.h>

using namespace Mantids30;
using namespace Mantids30::Network;

std::set<std::string> parseCommaSeparatedString(const std::string &input)
{
    std::set<std::string> result;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        if (!item.empty())
        {
            result.insert(item);
        }
    }
    return result;
}
Mantids30::Network::Servers::RESTful::Engine *Mantids30::Program::Config::RESTful_Engine::createRESTfulEngine(
                        boost::property_tree::ptree *config,
                        Logs::AppLog *log,
                        Logs::RPCLog *rpcLog,
                        const std::string &serviceName,
                        const std::string & defaultResourcePath,
                        uint64_t options,
                        const std::map<std::string, std::string> & vars
                        )
{
    bool usingTLS = config->get<bool>("UseTLS", true);

    std::shared_ptr<Sockets::Socket_Stream> sockWebListen;

    // Retrieve listen port and address from configuration
    uint16_t listenPort = config->get<uint16_t>("ListenPort", 8443);
    std::string listenAddr = config->get<std::string>("ListenAddr", "0.0.0.0");

    if (usingTLS == false && (options&REST_ENGINE_MANDATORY_SSL))
    {
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s", serviceName.c_str(), listenAddr.c_str(), listenPort, "TLS is required/mandatory for this service.");
        return nullptr;
    }

    if (usingTLS == true && (options&REST_ENGINE_NO_SSL))
    {
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s", serviceName.c_str(), listenAddr.c_str(), listenPort, "TLS is not available on this service.");
        return nullptr;
    }

    if (usingTLS)
    {
        auto tlsSocket = std::make_shared<Sockets::Socket_TLS>();

        // Set the default security level for the socket
        tlsSocket->tlsKeys.setSecurityLevel(-1);

        // Load public key from PEM file for TLS
        if (!tlsSocket->tlsKeys.loadPublicKeyFromPEMFile(config->get<std::string>("TLS.CertFile", "snakeoil.crt").c_str()))
        {
            log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s",serviceName.c_str(), listenAddr.c_str(), listenPort, "Bad TLS Public Key");
            return nullptr;
        }

        // Load private key from PEM file for TLS
        if (!tlsSocket->tlsKeys.loadPrivateKeyFromPEMFile(config->get<std::string>("TLS.KeyFile", "snakeoil.key").c_str()))
        {
            log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s",serviceName.c_str(), listenAddr.c_str(), listenPort, "Bad TLS Private Key");
            return nullptr;
        }

        sockWebListen = tlsSocket;
    }
    else
    {
        sockWebListen = std::make_shared<Sockets::Socket_TCP>();
    }

    sockWebListen->setUseIPv6(config->get<bool>("UseIPv6", false));

    // Start listening on the specified address and port
    if (sockWebListen->listenOn(listenPort, listenAddr.c_str()))
    {
        // Create and configure the web server instance
        Network::Servers::RESTful::Engine *webServer = new Network::Servers::RESTful::Engine();
        webServer->log = log;

        log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] %s service is now listening at @%s:%" PRIu16 "", (void*)webServer, serviceName.c_str(), listenAddr.c_str(), listenPort);

        // Setup the RPC Log:
        webServer->config.rpcLog = rpcLog;

        std::string resourcesPath = config->get<std::string>("ResourcesPath",defaultResourcePath);
        if ((options & REST_ENGINE_DISABLE_RESOURCES) == 0 || resourcesPath.empty())
        {
            log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Setting document root path to %s", (void*)webServer, resourcesPath.c_str());
            if (!webServer->config.setDocumentRootPath( resourcesPath ))
            {
                log->log0(__func__,Logs::LEVEL_CRITICAL, "[%p] Error locating web server resources at %s", (void*)webServer,resourcesPath.c_str() );
                return nullptr;
            }
        }

        // All the API will be accessible from this Origins...
        std::string rawOrigins = config->get<std::string>("API.Origins", "");

        if (!rawOrigins.empty())
            log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Setting permitted API origins to %s", (void*)webServer, rawOrigins.c_str());
        else
            log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] API origins won't be enforced", (void*)webServer);

        webServer->config.permittedAPIOrigins = parseCommaSeparatedString(rawOrigins);

        // All the API will be accessible from this Origins...
        std::string callbackAPIMethodName = config->get<std::string>("Login.CallbackMethodName", "callback");
        log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Setting API Login callback method name to %s", (void*)webServer, callbackAPIMethodName.c_str());
        webServer->config.callbackAPIMethodName = callbackAPIMethodName;

        // The login can be made from this origins (will receive)
        // Set the permitted origin (login IAM location Origin)
        std::string loginOrigins = config->get<std::string>("Login.Origins", "");
        log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Setting permitted login origins to %s", (void*)webServer, loginOrigins.c_str());
        webServer->config.permittedLoginOrigins = parseCommaSeparatedString(loginOrigins);
        // Set the login IAM location:
        std::string loginRedirectURL = config->get<std::string>("Login.RedirectURL", "/login");
        log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Setting default login redirect URL to %s", (void*)webServer, loginRedirectURL.c_str());
        webServer->config.defaultLoginRedirect = loginRedirectURL;

        if ( (options & REST_ENGINE_NO_JWT)==0 )
        {
            // JWT
            webServer->config.jwtSigner = Program::Config::JWT::createJWTSigner(log, config, "JWT" );
            webServer->config.jwtValidator = Program::Config::JWT::createJWTValidator(log, config, "JWT" );

            if (!webServer->config.jwtValidator)
            {
                log->log0(__func__, Logs::LEVEL_CRITICAL, "[%p] We need at least a JWT Validator.", (void*)webServer);
                return nullptr;
            }
        }

        // Setup the callbacks:
        webServer->callbacks.onProtocolInitializationFailure = handleProtocolInitializationFailure;
        webServer->callbacks.onClientAcceptTimeoutOccurred = handleClientAcceptTimeoutOccurred;
        webServer->callbacks.onClientConnectionLimitPerIPReached = handleClientConnectionLimitPerIPReached;

        // Use a thread pool or multi-threading based on configuration
        bool useThreadPool = config->get<bool>("Threads.UseThreadPool", false);
        uint32_t threadsCount = useThreadPool ?
            config->get<uint32_t>("Threads.PoolSize", 10) :
            config->get<uint32_t>("Threads.MaxThreads", 10000);

        log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Using %s with %u threads",
                  (void*)webServer,
                  useThreadPool ? "thread pool" : "multi-threading",
                  threadsCount);

        if (useThreadPool)
            webServer->setAcceptPoolThreaded(sockWebListen, threadsCount);
        else
            webServer->setAcceptMultiThreaded(sockWebListen, threadsCount);

        // WebServer Extras:
        if (config->find("Proxies") != config->not_found())
        {
            log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Loading proxies...", (void*)webServer);
            // Loading proxies...

            for (const auto& proxy : config->get_child("Proxies"))
            {
                std::string proxyPath = proxy.first;
                log->log0(__func__, Logs::LEVEL_INFO, "[%p] Loading proxy to path '%s' at %s Service",
                          (void*)webServer,
                          proxyPath.c_str(), serviceName.c_str());

                std::shared_ptr<Network::Servers::Web::ApiProxyParameters> param = ApiProxyConfig::createApiProxyParams(log, proxy.second, vars );

                if (param!=nullptr)
                {
                    webServer->config.dynamicRequestHandlersByRoute[proxyPath] = {&Network::Servers::Web::ApiProxy, param};
                }
            }
        }

        if (config->find("Redirections") != config->not_found())
        {
            log->log0(__func__, Logs::LEVEL_DEBUG, "[%p] Loading redirections...", (void*)webServer);
            // Loading redirections...

            for (const auto& redirection : config->get_child("Redirections"))
            {
                std::string path = redirection.first;
                std::string url = redirection.second.get_value<std::string>("/");

                log->log0(__func__, Logs::LEVEL_INFO, "[%p] Loading transparent redirection to path '%s' for URL '%s'",
                          (void*)webServer,
                          path.c_str(), url.c_str());

                webServer->config.redirections[path] = url;
            }
        }



        return webServer;
    }
    else
    {
        // Log the error if the web service fails to start
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Error creating %s Service @%s:%" PRIu16 ": %s",serviceName.c_str(), listenAddr.c_str(), listenPort, sockWebListen->getLastError().c_str());
        return nullptr;
    }
}

bool Program::Config::RESTful_Engine::handleProtocolInitializationFailure(
    void *data, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    if (!sock->isSecure())
        return true;

    Network::Servers::Web::APIEngineCore *core = (Network::Servers::Web::APIEngineCore *) data;

    std::shared_ptr<Sockets::Socket_TLS> secSocket = std::dynamic_pointer_cast<Sockets::Socket_TLS>(sock);

    for (const std::string &i : secSocket->getTLSErrorsAndClear())
    {
        if (!strstr(i.c_str(), "certificate unknown"))
            core->log->log1(__func__, sock->getRemotePairStr(), Program::Logs::LEVEL_ERR, "TLS: %s", i.c_str());
    }
    return true;
}

bool Program::Config::RESTful_Engine::handleClientAcceptTimeoutOccurred(
    void *data, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    Network::Servers::Web::APIEngineCore *core = (Network::Servers::Web::APIEngineCore *) data;

    core->log->log1(__func__, sock->getRemotePairStr(), Program::Logs::LEVEL_ERR, "RESTful Service Timed Out.");
    return true;
}

bool Program::Config::RESTful_Engine::handleClientConnectionLimitPerIPReached(
    void *data, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    Network::Servers::Web::APIEngineCore *core = (Network::Servers::Web::APIEngineCore *) data;

    core->log->log1(__func__, sock->getRemotePairStr(), Program::Logs::LEVEL_DEBUG, "Client Connection Limit Per IP Reached...");
    return true;
}
