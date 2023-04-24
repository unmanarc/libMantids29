#ifndef SERVER_WEBAPI_RESTFUL_H
#define SERVER_WEBAPI_RESTFUL_H

#include <Mantids29/Server_WebCore/apienginecore.h>
#include <Mantids29/DataFormat_JWT/jwt.h>
#include <memory>
#include "methodshandler.h"

namespace Mantids29 { namespace Network { namespace Servers { namespace RESTful {

class Engine : public Web::APIEngineCore
{
public:
    Engine();
    ~Engine();

    std::shared_ptr<DataFormat::JWT> m_jwtEngine;
    std::map<uint32_t,std::shared_ptr<MethodsHandler>> m_methodsHandler;

    // TODO: max variable size
protected:
    Web::APIClientHandler * createNewAPIClientHandler(APIEngineCore * webServer, Network::Sockets::Socket_Stream_Base * s) override;

private:
    static Json::Value revokeJWT(void * obj, const RESTful::Parameters &inputParameters );

};

}}}}
#endif // SERVER_WEBAPI_RESTFUL_H
