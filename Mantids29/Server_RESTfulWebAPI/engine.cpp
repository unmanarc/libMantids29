#include "engine.h"
#include "Mantids29/Helpers/json.h"
#include "clienthandler.h"

using namespace Mantids29;
using namespace Network::Servers;
using namespace Network::Servers::RESTful;


Engine::Engine()
{
    // This method handler will be used for IAM inter-process communication (like token revokations)
    std::shared_ptr<API::RESTful::MethodsHandler> handler = std::make_shared<API::RESTful::MethodsHandler>();

    API::RESTful::MethodsHandler::RESTfulAPIDefinition jwtRevokeDef;
    jwtRevokeDef.method = &revokeJWT;
    jwtRevokeDef.security.requiredAttributes = {"IAM"};
    jwtRevokeDef.obj = this;

    handler->addResource(API::RESTful::MethodsHandler::POST, "revokeJWT", jwtRevokeDef);
    m_methodsHandler[0] = handler;
}

Engine::~Engine()
{
}

Json::Value Engine::revokeJWT(void *obj, const API::RESTful::Parameters &inputParameters)
{
    std::string jwtSignature = Helpers::Encoders::decodeFromBase64(JSON_ASSTRING(inputParameters.pathParameters, "signature",""),true);
    time_t expirationTime = JSON_ASUINT64(inputParameters.pathParameters, "expiration",0);
    ((Engine *)obj)->m_jwtEngine->m_revocation.addToRevocationList( jwtSignature, expirationTime );

    return true;
}

Web::APIClientHandler *Engine::createNewAPIClientHandler(APIEngineCore * webServer, Network::Sockets::Socket_Stream_Base * s)
{
    auto clientHandler = new RESTful::ClientHandler(webServer,s);
    clientHandler->m_jwtEngine = m_jwtEngine;
    clientHandler->m_methodsHandler = m_methodsHandler;
    return clientHandler;
}
