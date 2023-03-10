#include "fastrpc2.h"
#include "Mantids29/Helpers/random.h"

#include <Mantids29/Auth/multi.h>
#include <Mantids29/Helpers/callbacks.h>
#include <Mantids29/Helpers/json.h>
#include <Mantids29/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids29/Net_Sockets/socket_tls.h>
#include <Mantids29/RPC_Common/methodsmanager.h>
#include <Mantids29/Threads/lock_shared.h>

#include <boost/algorithm/string/predicate.hpp>
#include <cstdint>
#include <memory>

using namespace Mantids29::Network::Sockets;
using namespace Mantids29::RPC::Fast;
using namespace Mantids29;
using namespace std;

using Ms = chrono::milliseconds;
using S = chrono::seconds;

// TODO: listar usuarios logeados, registrar usuarios logeados? inicio de sesion/fin de sesion...

void vrsyncRPCPingerThread( FastRPC2 * obj )
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "fRPC2:Pinger");
#endif

    while (obj->waitPingInterval())
    {
        obj->sendPings(); // send pings to every registered client...
    }
}

FastRPC2::FastRPC2(const string &appName, void * callbacksObj, uint32_t threadsCount, uint32_t taskQueues) :
    defaultLoginRPCClient(callbacksObj), defaultMethodsManagers(appName),
    parameters(&defaultLoginRPCClient,&defaultMethodsManagers,&defaultAuthDomain)
{
    threadPool = new Mantids29::Threads::Pool::ThreadPool(threadsCount, taskQueues);

    finished = false;
//    overwriteObject = nullptr;

    // Configure current methods:
  /*  currentMethodsManagers = ;
    currentLoginRPCClient = ;
    currentAuthDomains = ;*/

    // Configure the domain "" with the auth manager of the fast rpc client for authentication...
    defaultAuthDomain.addDomain("",defaultLoginRPCClient.getRemoteAuthManager());

    threadPool->start();
    pinger = thread(vrsyncRPCPingerThread,this);
}

FastRPC2::~FastRPC2()
{
    // Set pings to cease (the current one will continue)
    finished=true;

    // Notify that pings should stop now... This can take a while (while pings are cycled)...
    {
        unique_lock<mutex> lk(mtPing);
        cvPing.notify_all();
    }

    // Wait until the loop ends...
    pinger.join();

    delete threadPool;


}

void FastRPC2::stop()
{
    threadPool->stop();
}

void FastRPC2::sendPings()
{
    // This will create some traffic:
    auto keys = connectionsByKeyId.getKeys();
    for (const auto & i : keys)
    {
        // Avoid to ping more hosts during program finalization...
        if (finished)
            return;
        // Run unexistant remote function
        runRemoteRPCMethod(i,"_pingNotFound_",{},nullptr,false);
    }
}

bool FastRPC2::waitPingInterval()
{
    unique_lock<mutex> lk(mtPing);
    if (cvPing.wait_for(lk,S(parameters.pingIntervalInSeconds)) == cv_status::timeout )
    {
        return true;
    }
    return false;
}

int FastRPC2::processAnswer(FastRPC2::Connection * connection)
{
    CallbackDefinitions * callbacks = ((CallbackDefinitions *)connection->callbacks);

    uint32_t maxAlloc = parameters.maxMessageSize;
    uint64_t requestId;
    uint8_t executionStatus;
    char * payloadBytes;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=connection->stream->readU<uint64_t>();
    if (!requestId)
    {
        return -1;
    }
    ////////////////////////////////////////////////////////////
    // READ IF EXECUTED.
    executionStatus=connection->stream->readU<uint8_t>();

    // READ THE PAYLOAD...
    payloadBytes = connection->stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    if (true)
    {
        unique_lock<mutex> lk(connection->mtAnswers);
        if ( connection->pendingRequests.find(requestId) != connection->pendingRequests.end())
        {
            connection->executionStatus[requestId] = executionStatus;

            Mantids29::Helpers::JSONReader2 reader;
            bool parsingSuccessful = reader.parse( payloadBytes, connection->answers[requestId] );
            if (parsingSuccessful)
            {
                // Notify that there is a new answer... everyone have to check if it's for him.
                connection->cvAnswers.notify_all();
            }
            else
            {
                // if not able to answer, please remove from answers...
                // TODO: notify this malformed data...
                connection->answers.erase(requestId);
                connection->executionStatus.erase(requestId);
                connection->pendingRequests.erase(requestId);
            }
        }
        else
        {
            CALLBACK(callbacks->CB_RemotePeer_UnexpectedAnswerReceived)(connection,payloadBytes);
        }
    }

    delete [] payloadBytes;
    return 1;
}

int FastRPC2::processQuery(Socket_TLS *stream, const string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket, FastRPC2::SessionPTR * sessionHolder)
{
    uint32_t maxAlloc = parameters.maxMessageSize;
    uint64_t requestId;
    char * payloadBytes;
    bool ok;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=stream->readU<uint64_t>();
    if (!requestId)
    {
        return -1;
    }
    // READ THE METHOD NAME.
    string methodName = stream->readStringEx<uint8_t>(&ok);
    if (!ok)
    {
        return -2;
    }
    // READ THE PAYLOAD...
    payloadBytes = stream->readBlockWAllocEx<uint32_t>(&maxAlloc);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    // Process / Inject task:
    Mantids29::Helpers::JSONReader2 reader;
    FastRPC2::TaskParameters * params = new FastRPC2::TaskParameters;
    params->sessionHolder = sessionHolder;
    params->currentMethodsManagers = parameters.currentMethodsManagers;
    params->currentAuthDomains = parameters.currentAuthDomains;
    params->requestId = requestId;
    params->methodName = methodName;
    params->ipAddr = stream->getRemotePairStr();
    params->cn = stream->getTLSPeerCN();
    params->done = mtDone;
    params->mtSocket = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->maxMessageSize = parameters.maxMessageSize;
    params->callbacks = &callbacks;

    bool parsingSuccessful = reader.parse( payloadBytes, params->payload );
    delete [] payloadBytes;

    if ( !parsingSuccessful )
    {
        // Bad Incomming JSON... Disconnect
        delete params;
        return -3;
    }
    else
    {
        params->done->lockShared();

        void (*currentTask)(void *) = executeRPCTask;

        if (params->methodName == "SESSION.LOGIN")
            currentTask = executeRPCLogin;
        else if (params->methodName == "SESSION.LOGOUT")
            currentTask = executeRPCLogout;
        else if (params->methodName == "SESSION.CHPASSWD")
            currentTask = executeRPCChangePassword;
        else if (params->methodName == "SESSION.TSTPASSWD")
            currentTask = executeRPCTestPassword;
        else if (params->methodName == "SESSION.LISTPASSWD")
            currentTask = executeRPCListPassword;

        if (!threadPool->pushTask(currentTask,params,parameters.queuePushTimeoutInMS,priority,key))
        {
            // Can't push the task in the queue. Null answer.
            CALLBACK(callbacks.CB_Incomming_DroppingOnFullQueue)(params);
            sendRPCAnswer(params,"",3);
            params->done->unlockShared();
            delete params;
        }
    }
    return 1;
}

json FastRPC2::runRemoteLogin(const std::string &connectionKey, const std::string &user, const Authentication::Data &authData, const std::string &domain, json *error)
{
    json jAuthData;
    jAuthData["user"] = user;
    jAuthData["domain"] = domain;
    jAuthData["authData"] = authData.toJson();
    return runRemoteRPCMethod(connectionKey, "SESSION.LOGIN", {}, error, true, true);
}

json FastRPC2::runRemoteChangePassword(const std::string &connectionKey, const Authentication::Data &oldAuthData, const Authentication::Data &newAuthData, json *error)
{
    json jAuthData;
    jAuthData["newAuth"] = newAuthData.toJson();
    jAuthData["oldAuth"] = oldAuthData.toJson();
    return runRemoteRPCMethod(connectionKey, "SESSION.CHPASSWD", {}, error, true, true);
}

json FastRPC2::runRemoteTestPassword(const std::string &connectionKey, const Authentication::Data &authData, json *error)
{
    json jAuthData;
    jAuthData["auth"] = authData.toJson();
    return runRemoteRPCMethod(connectionKey, "SESSION.TSTPASSWD", {}, error, true, true);
}

json FastRPC2::runRemoteListPasswords(const std::string &connectionKey, const Authentication::Data &authData, json *error)
{
    return runRemoteRPCMethod(connectionKey, "SESSION.LISTPASSWD", {}, error, true, true);
}

bool FastRPC2::runRemoteLogout( const string &connectionKey, json *error )
{
    json x = runRemoteRPCMethod(connectionKey, "SESSION.LOGOUT", {}, error, true, true);
    return JSON_ASBOOL(x,"ok",false);
}

int FastRPC2::connectionHandler(Network::Sockets::Socket_TLS *stream, bool remotePeerIsServer, const char *remotePair)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "VSRPC:ProcStr");
#endif

    int ret = 1;

    Threads::Sync::Mutex_Shared mtDone;
    Threads::Sync::Mutex mtSocket;

    FastRPC2::Connection * connection = new FastRPC2::Connection;
    connection->callbacks = &callbacks;
    connection->mtSocket = &mtSocket;
    connection->key = !remotePeerIsServer? stream->getTLSPeerCN() + "?" + Helpers::Random::createRandomHexString(8) : stream->getTLSPeerCN();
    connection->stream = stream;

    // TODO: multiple connections from the same key?
    if (!connectionsByKeyId.addElement(connection->key,connection))
    {
        delete connection;
        return -2;
    }

    stream->setReadTimeout(parameters.rwTimeoutInSeconds);
    stream->setWriteTimeout(parameters.rwTimeoutInSeconds);

    FastRPC2::SessionPTR session;

    while (ret>0)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        bool readOK;
        switch (stream->readU<uint8_t>(&readOK))
        {
        case 'A':
            // Process Answer, incomming answer for query, report to the caller...
            ret = processAnswer(connection);
            break;
        case 'Q':
            // Process Query, incomming query...
            ret = processQuery(stream,connection->key,parameters.keyDistFactor,&mtDone,&mtSocket,&session);
            break;
        case 0:
            // Remote shutdown
            // TODO: clean up on exit and send the signal back?
            if (!readOK) // <- read timeout.
                ret = -101;
            else
                ret = 0;
            break;
        default:
            // Invalid protocol.
            ret = -100;
            break;
        }
        //        connection->lastReceivedData = time(nullptr);
    }

    // Wait until all task are done.
    mtDone.lock();
    mtDone.unlock();

    stream->shutdownSocket();

    connection->terminated = true;
    connection->cvAnswers.notify_all();
    connectionsByKeyId.destroyElement(connection->key);

    return ret;
}

Mantids29::Authentication::Reason temporaryAuthentication(FastRPC2::TaskParameters * params ,const std::string & userName, const std::string & domainName, const Authentication::Data &authData)
{
    Mantids29::Authentication::Reason eReason;

    auto auth = params->currentAuthDomains->openDomain(domainName);
    if (!auth)
        eReason = Mantids29::Authentication::REASON_INVALID_DOMAIN;
    else
    {
        Mantids29::Authentication::ClientDetails clientDetails;
        clientDetails.ipAddress = params->ipAddr;
        clientDetails.tlsCommonName = params->cn;
        clientDetails.userAgent = "FastRPC2";

        eReason = auth->authenticate( params->currentMethodsManagers->getAppName(), clientDetails, userName,authData.m_password,authData.m_passwordIndex); // Authenticate in a non-persistent fashion.
        params->currentAuthDomains->releaseDomain(domainName);
    }

    return eReason;
}

void FastRPC2::executeRPCTask(void *vTaskParams)
{
    TaskParameters * taskParams = (TaskParameters *)(vTaskParams);
    CallbackDefinitions * callbacks = ((CallbackDefinitions *)taskParams->callbacks);
    std::shared_ptr<Authentication::Session> session = taskParams->sessionHolder->get();

    json response;
    json rsp;
    response["ret"] = 0;

    Mantids29::Helpers::JSONReader2 reader;

    std::string  userName   = JSON_ASSTRING(taskParams->payload["extraAuth"],"user","");
    std::string domainName  = JSON_ASSTRING(taskParams->payload["extraAuth"],"domain","");

    Authentication::Multi extraAuths;
    extraAuths.setJson(taskParams->payload["extraAuth"]["data"]);

    // If there is a session, overwrite the user/domain inputs...
    if (session)
    {
        userName = session->getAuthUser();
        domainName = session->getAuthenticatedDomain();
    }

    if (!taskParams->currentAuthDomains)
    {
        return;
    }

    if (taskParams->currentMethodsManagers->getMethodRequireFullAuth(taskParams->methodName) && !session)
    {
        CALLBACK(callbacks->CB_MethodExecution_RequiredSessionNotProvided)(callbacks->obj, taskParams);
        return;
    }

    // TODO: what happens if we are given with unhandled but valid auths that should not be validated...?
    // Get/Pass the temporary authentications for null and not-null sessions:
    std::set<uint32_t> extraTmpIndexes;
    for (const uint32_t & passIdx : extraAuths.getAvailableIndices())
    {
        Mantids29::Authentication::Reason authReason=temporaryAuthentication( taskParams,
                                                                            userName,
                                                                            domainName,
                                                                            extraAuths.getAuthentication(passIdx) );

        // Include the pass idx in the Extra TMP Index.
        if ( Mantids29::Authentication::IS_PASSWORD_AUTHENTICATED( authReason ) )
        {
            CALLBACK(callbacks->CB_MethodExecution_ValidatedTemporaryAuthFactor)(callbacks->obj, taskParams, passIdx, authReason);
            extraTmpIndexes.insert(passIdx);
        }
        else
        {
            CALLBACK(callbacks->CB_MethodExecution_FailedValidationOnTemporaryAuthFactor)(callbacks->obj, taskParams, passIdx, authReason);
        }
    }

    bool found = false;

    auto authorizer = taskParams->currentAuthDomains->openDomain(domainName);
    if (authorizer)
    {
        json reasons;

        // Validate that the RPC method is ready to go (fully authorized and no password is expired).
        auto i = taskParams->currentMethodsManagers->validateRPCMethodPerms( authorizer,  session.get(), taskParams->methodName, extraTmpIndexes, &reasons );

        taskParams->currentAuthDomains->releaseDomain(domainName);

        switch (i)
        {
        case MethodsManager::VALIDATION_OK:
        {
            if (session)
                session->updateLastActivity();

            // Report:
            CALLBACK(callbacks->CB_MethodExecution_Starting)(callbacks->obj, taskParams, taskParams->payload);

            auto start = chrono::high_resolution_clock::now();
            auto finish = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = finish - start;

            switch (taskParams->currentMethodsManagers->runRPCMethod(taskParams->currentAuthDomains,domainName, session.get(), taskParams->methodName, taskParams->payload, &rsp))
            {
            case Mantids29::RPC::MethodsManager::METHOD_RET_CODE_SUCCESS:

                finish = chrono::high_resolution_clock::now();
                elapsed = finish - start;

                CALLBACK(callbacks->CB_MethodExecution_ExecutedOK)(callbacks->obj, taskParams, elapsed.count(), rsp);

                found = true;
                response["ret"] = 200;
                break;
            case Mantids29::RPC::MethodsManager::METHOD_RET_CODE_METHODNOTFOUND:

                CALLBACK(callbacks->CB_MethodExecution_MethodNotFound)(callbacks->obj, taskParams);
                response["ret"] = 404;
                break;
            case Mantids29::RPC::MethodsManager::METHOD_RET_CODE_INVALIDDOMAIN:
                // This code should never be executed... <
                CALLBACK(callbacks->CB_MethodExecution_DomainNotFound)(callbacks->obj, taskParams);
                response["ret"] = 404;
                break;
            default:
                CALLBACK(callbacks->CB_MethodExecution_UnknownError)(callbacks->obj, taskParams);
                response["ret"] = 401;
                break;
            }
        }break;
        case MethodsManager::VALIDATION_NOTAUTHORIZED:
        {
            // not enough permissions.
            CALLBACK(callbacks->CB_MethodExecution_NotAuthorized)(callbacks->obj, taskParams, reasons);
            response["auth"]["reasons"] = reasons;
            response["ret"] = 401;
        }break;
        case MethodsManager::VALIDATION_METHODNOTFOUND:
        default:
        {
            CALLBACK(callbacks->CB_MethodExecution_MethodNotFound)(callbacks->obj, taskParams);
            response["ret"] = 404;
        }break;
        }
    }
    else
    {
        CALLBACK(callbacks->CB_MethodExecution_DomainNotFound)(callbacks->obj, taskParams);
        // Domain Not found.
        response["ret"] = 404;
    }

    //Json::StreamWriterBuilder builder;
    //builder.settings_["indentation"] = "";

    //
    response["payload"] = rsp;
    sendRPCAnswer(taskParams,response.toStyledString(),found?2:4);
    taskParams->done->unlockShared();
}

void FastRPC2::executeRPCLogin(void *taskData)
{
    FastRPC2::TaskParameters * taskParams = (FastRPC2::TaskParameters *)(taskData);
    CallbackDefinitions * callbacks = ((CallbackDefinitions *)taskParams->callbacks);

    // CREATE NEW SESSION:
    json response;
    Mantids29::Authentication::Reason authReason = Authentication::REASON_INTERNAL_ERROR;

    auto session = taskParams->sessionHolder->get();

    std::string user = JSON_ASSTRING(taskParams->payload,"user","");
    std::string domain = JSON_ASSTRING(taskParams->payload,"domain","");
    Authentication::Data authData;
    authData.setJson(taskParams->payload["authData"]);
    std::map<uint32_t,std::string> stAccountPassIndexesUsedForLogin;

    if (session == nullptr && authData.m_passwordIndex!=0)
    {
        // Why are you trying to authenticate this way?
    }
    else
    {
        // PROCEED THEN....

        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(domain);
        if (domainAuthenticator)
        {
            Mantids29::Authentication::ClientDetails clientDetails;
            clientDetails.ipAddress = taskParams->ipAddr;
            clientDetails.tlsCommonName = taskParams->cn;
            clientDetails.userAgent = "FastRPC2 AGENT";

            authReason = domainAuthenticator->authenticate(taskParams->currentMethodsManagers->getAppName(),clientDetails,
                                                           user,authData.m_password,
                                                           authData.m_passwordIndex,
                                                           Mantids29::Authentication::MODE_PLAIN,
                                                           "",
                                                           &stAccountPassIndexesUsedForLogin);
            taskParams->currentAuthDomains->releaseDomain(domain);
        }
        else
        {
            CALLBACK(callbacks->CB_Login_InvalidDomain)(callbacks->obj, taskParams, domain);
            authReason = Mantids29::Authentication::REASON_INVALID_DOMAIN;
        }

        if ( Mantids29::Authentication::IS_PASSWORD_AUTHENTICATED( authReason ) )
        {
            // If not exist an authenticated session, create a new one.
            if (!session)
            {
                session = taskParams->sessionHolder->create(taskParams->currentMethodsManagers->getAppName());
                if (session)
                {
                    session->setPersistentSession(true);
                    session->registerPersistentAuthentication(user,domain,authData.m_passwordIndex,authReason);

                    // The first pass/time the list of idx should be filled into.
                    if (authData.m_passwordIndex==0)
                        session->setRequiredBasicAuthenticationIndices(stAccountPassIndexesUsedForLogin);
                }
            }
            else
            {
                // If exist, just register the current authentication into that session and return the current sessionid
                session->registerPersistentAuthentication(user,domain,authData.m_passwordIndex,authReason);
            }
        }
        else
        {
            CALLBACK(callbacks->CB_Login_AuthenticationFailed)(callbacks->obj, taskParams, user,domain,authReason);
        }

        response["txt"] = getReasonText(authReason);
        response["val"] = static_cast<Json::UInt>(authReason);
        response["nextPassReq"] = false;

        auto i = session->getNextRequiredAuthenticationIndex();
        if (i.first != 0xFFFFFFFF)
        {
            // No next login idx.
            response.removeMember("nextPassReq");
            response["nextPassReq"]["idx"] = i.first;
            response["nextPassReq"]["desc"] = i.second;
            CALLBACK(callbacks->CB_Login_HalfAuthenticationRequireNextFactor)(callbacks->obj, taskParams, response);
        }
        else
        {
            CALLBACK(callbacks->CB_Login_LoggedIn)(callbacks->obj, taskParams, response, user, domain);
        }
    }

    sendRPCAnswer(taskParams,response.toStyledString(),2);
    taskParams->done->unlockShared();
}

void FastRPC2::executeRPCLogout(void *taskData)
{
    FastRPC2::TaskParameters * params = (FastRPC2::TaskParameters *)(taskData);
    json response;

    response["ok"] = params->sessionHolder->destroy();

    sendRPCAnswer(params,response.toStyledString(),2);
    params->done->unlockShared();
}

void FastRPC2::executeRPCChangePassword(void *taskData)
{
    FastRPC2::TaskParameters * taskParams = (FastRPC2::TaskParameters *)(taskData);
    CallbackDefinitions * callbacks = ((CallbackDefinitions *)taskParams->callbacks);

    json response;
    response["ok"] = false;

    auto session = taskParams->sessionHolder->get();
    Authentication::Data newAuth, oldAuth;

    if (!session || !session->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS))
    {
        // TODO: what if expired?
        response["reason"] = "BAD_SESSION";
    }
    else if (!newAuth.setJson(taskParams->payload["newAuth"]) || !newAuth.setJson(taskParams->payload["oldAuth"]))
    {
        // Error parsing credentials...
        response["reason"] = "ERROR_PARSING_AUTH";
    }
    else if (oldAuth.m_passwordIndex!=newAuth.m_passwordIndex)
    {
        response["reason"] = "IDX_DIFFER";
    }
    else
    {
        uint32_t credIdx = newAuth.m_passwordIndex;

        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(session->getAuthenticatedDomain());
        if (domainAuthenticator)
        {
            Mantids29::Authentication::ClientDetails clientDetails;
            clientDetails.ipAddress = taskParams->ipAddr;
            clientDetails.tlsCommonName = taskParams->cn;
            clientDetails.userAgent = "FastRPC2 AGENT";

            auto authReason = domainAuthenticator->authenticate(taskParams->currentMethodsManagers->getAppName(),clientDetails,session->getAuthUser(),oldAuth.m_password,credIdx);

            if (IS_PASSWORD_AUTHENTICATED(authReason))
            {
                // TODO: alternative/configurable password storage...
                // TODO: check password policy.
                Mantids29::Authentication::Secret newSecretData = Mantids29::Authentication::createNewSecret(newAuth.m_password,Mantids29::Authentication::FN_SSHA256);

                response["ok"] = domainAuthenticator->accountChangeAuthenticatedSecret(taskParams->currentMethodsManagers->getAppName(),
                                                                                                              session->getAuthUser(),
                                                                                                              credIdx,
                                                                                                              oldAuth.m_password,
                                                                                                              newSecretData,
                                                                                                              clientDetails
                                                                                                              );

                if ( JSON_ASBOOL(response,"ok",false) == true)
                {
                    response["reason"] = "OK";
                    CALLBACK(callbacks->CB_PasswordChange_RequestedOK)(callbacks->obj, taskParams, session->getAuthUser(), session->getAuthenticatedDomain(), credIdx);
                }
                else
                {
                    response["reason"] = "ERROR";
                    CALLBACK(callbacks->CB_PasswordChange_RequestFailed)(callbacks->obj, taskParams, session->getAuthUser(), session->getAuthenticatedDomain(), credIdx);
                }
            }
            else
            {
                CALLBACK(callbacks->CB_PasswordChange_BadCredentials)(callbacks->obj, taskParams, session->getAuthUser(), session->getAuthenticatedDomain(), credIdx, authReason);

                // Mark to Destroy the session if the chpasswd is invalid...
                // DEAUTH:
                session = nullptr;
                taskParams->sessionHolder->destroy();
                response["reason"] = "UNAUTHORIZED";
            }

            taskParams->currentAuthDomains->releaseDomain(session->getAuthenticatedDomain());
        }
        else
        {
            response["reason"] = "DOMAIN_NOT_FOUND";
            CALLBACK(callbacks->CB_PasswordChange_InvalidDomain)(callbacks->obj, taskParams, session->getAuthenticatedDomain(), credIdx);
        }
    }

    sendRPCAnswer(taskParams,response.toStyledString(),2);
    taskParams->done->unlockShared();
}

void FastRPC2::executeRPCTestPassword(void *taskData)
{
    FastRPC2::TaskParameters * taskParams = (FastRPC2::TaskParameters *)(taskData);
    CallbackDefinitions * callbacks = ((CallbackDefinitions *)taskParams->callbacks);

    Authentication::Data auth;
    json response;
    auto session = taskParams->sessionHolder->get();
    response["ok"] = false;

    // TODO:
    if (!session || !session->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS))
    {
        // TODO: what if expired?
        response["reason"] = "BAD_SESSION";
    }
    else if (!auth.setJson(taskParams->payload["auth"]))
    {
        // Error parsing credentials...
        response["reason"] = "ERROR_PARSING_AUTH";
    }
    else
    {
        uint32_t credIdx = auth.m_passwordIndex;

        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(session->getAuthenticatedDomain());
        if (domainAuthenticator)
        {
            Mantids29::Authentication::ClientDetails clientDetails;
            clientDetails.ipAddress = taskParams->ipAddr;
            clientDetails.tlsCommonName = taskParams->cn;
            clientDetails.userAgent = "FastRPC2 AGENT";

            auto authReason = domainAuthenticator->authenticate(taskParams->currentMethodsManagers->getAppName(),clientDetails,session->getAuthUser(),auth.m_password,credIdx);
            if (IS_PASSWORD_AUTHENTICATED(authReason))
            {
                response["ok"] = true;
                CALLBACK(callbacks->CB_PasswordValidation_OK)(callbacks->obj, taskParams, session->getAuthUser(),session->getAuthenticatedDomain(),credIdx );
            }
            else
            {
                // DEAUTH:
                session = nullptr;
                taskParams->sessionHolder->destroy();

                CALLBACK(callbacks->CB_PasswordValidation_Failed)(callbacks->obj, taskParams, session->getAuthUser(),session->getAuthenticatedDomain(),credIdx, authReason );
            }

            taskParams->currentAuthDomains->releaseDomain(session->getAuthenticatedDomain());
        }
        else
        {
            response["reason"] = "BAD_DOMAIN";
            CALLBACK(callbacks->CB_PasswordValidation_InvalidDomain)(callbacks->obj, taskParams, session->getAuthenticatedDomain(), credIdx);
        }

    }

    sendRPCAnswer(taskParams,response.toStyledString(),2);
    taskParams->done->unlockShared();
}

void FastRPC2::executeRPCListPassword(void *taskData)
{
    FastRPC2::TaskParameters * taskParams = (FastRPC2::TaskParameters *)(taskData);
    //TaskCallbacks * callbacks = ((TaskCallbacks *)taskParams->callbacks);

    json response;
    response["ok"] = false;
    auto session = taskParams->sessionHolder->get();

    if (!session || !session->isFullyAuthenticated(Mantids29::Authentication::Session::CHECK_ALLOW_EXPIRED_PASSWORDS))
    {
        response["reason"] = "BAD_SESSION";
    }
    else
    {
        auto domainAuthenticator = taskParams->currentAuthDomains->openDomain(session->getAuthenticatedDomain());
        if (domainAuthenticator)
        {
            std::map<uint32_t, Mantids29::Authentication::Secret_PublicData> publics = domainAuthenticator->getAccountAllSecretsPublicData(session->getAuthUser());
            response["ok"] = true;

            uint32_t ix=0;
            for (const auto & i : publics)
            {
                response["list"][ix]["badAtttempts"] = i.second.badAttempts;
                response["list"][ix]["forceExpiration"] = i.second.forceExpiration;
                response["list"][ix]["nul"] = i.second.nul;
                response["list"][ix]["passwordFunction"] = i.second.passwordFunction;
                response["list"][ix]["expiration"] = (Json::UInt64)i.second.expiration;
                response["list"][ix]["description"] = i.second.description;
                response["list"][ix]["isExpired"] = i.second.isExpired();
                response["list"][ix]["isRequiredAtLogin"] = i.second.requiredAtLogin;
                response["list"][ix]["isLocked"] = i.second.locked;
                response["list"][ix]["idx"] = i.first;
                ix++;
            }
        }
        else
            response["reason"] = "BAD_DOMAIN";
    }

    sendRPCAnswer(taskParams,response.toStyledString(),2);
    taskParams->done->unlockShared();
}

void FastRPC2::sendRPCAnswer(FastRPC2::TaskParameters *params, const string &answer,uint8_t execution)
{
    // Send a block.
    params->mtSocket->lock();
    if (    params->streamBack->writeU<uint8_t>('A') && // ANSWER
            params->streamBack->writeU<uint64_t>(params->requestId) &&
            params->streamBack->writeU<uint8_t>(execution) &&
            params->streamBack->writeStringEx<uint32_t>(answer.size()<=params->maxMessageSize?answer:"",params->maxMessageSize ) )
    {
    }
    params->mtSocket->unlock();
}

json FastRPC2::runRemoteRPCMethod(const string &connectionKey, const string &methodName, const json &payload, json *error, bool retryIfDisconnected, bool passSessionCommands)
{
    json r;

    if (!passSessionCommands && boost::starts_with(methodName,"SESSION."))
        return r;

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    string output = Json::writeString(builder, payload);

    if (output.size()>parameters.maxMessageSize)
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 1;
            (*error)["errorMessage"] = "Payload exceed the Maximum Message Size.";
        }
        return r;
    }

    FastRPC2::Connection * connection;

    uint32_t _tries=0;
    while ( (connection=(FastRPC2::Connection *)connectionsByKeyId.openElement(connectionKey))==nullptr )
    {
        _tries++;
        if (_tries >= parameters.remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            CALLBACK(callbacks.CB_RemotePeer_Disconnected)(connectionKey,methodName,payload);
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 2;
                (*error)["errorMessage"] = "Abort after remote peer not found/connected.";
            }
            return r;
        }
        sleep(1);
    }

    uint64_t requestId;
    // Create a request ID.
    connection->mtReqIdCt.lock();
    requestId = connection->requestIdCounter++;
    connection->mtReqIdCt.unlock();

    if (1)
    {
        unique_lock<mutex> lk(connection->mtAnswers);
        // Create authorization to be inserted:
        connection->pendingRequests.insert(requestId);
    }

    connection->mtSocket->lock();
    if (    connection->stream->writeU<uint8_t>('Q') && // QUERY FOR ANSWER
            connection->stream->writeU<uint64_t>(requestId) &&
            connection->stream->writeStringEx<uint8_t>(methodName) &&
            connection->stream->writeStringEx<uint32_t>( output,parameters.maxMessageSize ) )
    {
    }
    connection->mtSocket->unlock();

    // Time to wait for answers...
    for (;;)
    {
        unique_lock<mutex> lk(connection->mtAnswers);

        // Process multiple signals until our answer comes...

        if (connection->cvAnswers.wait_for(lk,Ms(parameters.remoteExecutionTimeoutInMS)) == cv_status::timeout )
        {
            // break by timeout. (no answer)
            CALLBACK(callbacks.CB_Outgoing_ExecutionTimedOut)(connectionKey,methodName,payload);

            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 3;
                (*error)["errorMessage"] = "Remote Execution Timed Out: No Answer Received.";
            }
            break;
        }

        if ( lk.owns_lock() && connection->answers.find(requestId) != connection->answers.end())
        {
            // break by element found. (answer)
            uint8_t executionStatus = connection->executionStatus[requestId];
            r = connection->answers[requestId];
            if (error)
            {
                switch (executionStatus)
                {
                case 2:
                    (*error)["succeed"] = true;
                    (*error)["errorId"] = 0;
                    (*error)["errorMessage"] = "Execution OK.";
                    break;
                case 3:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 4;
                    (*error)["errorMessage"] = "Remote Execution Failed: Full Queue.";
                    break;
                case 4:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 5;
                    (*error)["errorMessage"] = "Remote Execution Failed: Method Not Found.";
                    break;
                default:
                    (*error)["succeed"] = false;

                }
            }
            break;
        }

        if ( lk.owns_lock() && connection->terminated )
        {
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 6;
                (*error)["errorMessage"] = "Connection is terminated: No Answer Received.";
            }
            break;
        }
    }

    if (1)
    {
        unique_lock<mutex> lk(connection->mtAnswers);
        // Revoke authorization to be inserted, clean results...
        connection->answers.erase(requestId);
        connection->executionStatus.erase(requestId);
        connection->pendingRequests.erase(requestId);
    }

    connectionsByKeyId.releaseElement(connectionKey);

    if (error)
    {
        if (!error->isMember("succeed"))
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 99;
            (*error)["errorMessage"] = "Unknown Error.";
        }
    }

    return r;
}

bool FastRPC2::runRemoteClose(const string &connectionKey)
{
    bool r = false;

    FastRPC2::Connection * connection;
    if ((connection=(FastRPC2::Connection *)connectionsByKeyId.openElement(connectionKey))!=nullptr)
    {

        connection->mtSocket->lock();
        if (    connection->stream->writeU<uint8_t>(0) )
        {
        }
        connection->mtSocket->unlock();

        connectionsByKeyId.releaseElement(connectionKey);
    }
    else
    {
        CALLBACK(callbacks.CB_RemotePeer_Disconnected)(connectionKey,"",{});
    }
    return r;
}

set<string> FastRPC2::getConnectionKeys()
{
    return connectionsByKeyId.getKeys();
}

bool FastRPC2::checkConnectionKey(const string &connectionKey)
{
    return connectionsByKeyId.isMember(connectionKey);
}



