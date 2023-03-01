#include "manager_remote.h"

using namespace Mantids29::Authentication;
using namespace Mantids29::RPC;

#define CKEY "SERVER"

Manager_Remote::Manager_Remote()
{
    m_fastRPC = new FastRPCImpl(1);
}

Manager_Remote::~Manager_Remote()
{
    delete m_fastRPC;
    m_fastRPC = nullptr;
}

Reason Manager_Remote::authenticate(const std::string &, const ClientDetails &clientDetails, const std::string &sAccountName, const std::string &sPassword, uint32_t passIndex, Mode authMode, const std::string &challengeSalt, std::map<uint32_t, std::string> *accountPassIndexesUsedForLogin)
{
    json payload;

    payload["clientDetails"]["ipAddr"] = clientDetails.ipAddress;
    payload["clientDetails"]["extraData"] = clientDetails.extraData;
    payload["clientDetails"]["tlsCN"] = clientDetails.tlsCommonName;
    payload["clientDetails"]["userAgent"] = clientDetails.userAgent;

    payload["accountName" ] = sAccountName;
    payload["password"] = sPassword;
    payload["passIndex"] = passIndex;
    payload["authMode"] = getStringFromAuthMode(authMode);
    payload["challengeSalt"] = challengeSalt;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"authenticate",payload,nullptr), jNull;

    if (jNull == answer)
    {
        return REASON_INTERNAL_ERROR;
    }

    if (accountPassIndexesUsedForLogin && !answer["accountPassIndexesUsedForLogin"].isNull())
    {
        for ( size_t i=0; i<answer["accountPassIndexesUsedForLogin"].size();i++ )
        {
            json v = answer["accountPassIndexesUsedForLogin"][(int)i];

            uint32_t key = JSON_ASUINT(v,"idx",0);
            std::string txt = JSON_ASSTRING(v,"txt","");

            (*accountPassIndexesUsedForLogin)[key] = txt;
        }
    }

    return (Reason)JSON_ASUINT(answer,"retCode",0);
}

bool Manager_Remote::accountChangeAuthenticatedSecret(const std::string &, const std::string &sAccountName, uint32_t passIndex, const std::string &sCurrentPassword, const Secret &newPasswordData, const ClientDetails &clientDetails, Mode authMode, const std::string &challengeSalt)
{
    json payload;

    payload["clientDetails"]["ipAddr"] = clientDetails.ipAddress;
    payload["clientDetails"]["extraData"] = clientDetails.extraData;
    payload["clientDetails"]["tlsCN"] = clientDetails.tlsCommonName;
    payload["clientDetails"]["userAgent"] = clientDetails.userAgent;

    payload["accountName" ] = sAccountName;
    payload["passIndex"] = passIndex;
    payload["currentPassword"] = sCurrentPassword;
    payload["authMode"] = getStringFromAuthMode(authMode);
    payload["challengeSalt"] = challengeSalt;


    for (const auto & i : newPasswordData.getMap())
    {
        payload["newSecret"][i.first] = i.second;
    }

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountChangeAuthenticatedSecret",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

std::map<uint32_t, Secret_PublicData> Manager_Remote::getAccountAllSecretsPublicData(const std::string &sAccountName)
{
    std::map<uint32_t, Secret_PublicData> r;

    json payload;

    payload["accountName" ] = sAccountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"getAccountAllSecretsPublicData",payload,nullptr);

    for (const auto & i : answer.getMemberNames())
    {
        uint32_t idx = strtoul(i.c_str(),0,10);
        std::map<std::string,std::string> values;
        for ( const auto & j  : answer[i].getMemberNames() )
        {
            values[j] = JSON_ASSTRING(answer[i],j,"");
        }
        r[idx].fromMap(values);
    }
    return r;
}

std::set<uint32_t> Manager_Remote::passIndexesUsedByAccount(const std::string & sAccountName )
{
    json payload;
    std::set<uint32_t> r;
    payload["accountName" ] = sAccountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"passIndexesUsedByAccount",payload,nullptr), jNull;

    if (jNull != answer)
    {
        for ( uint32_t i = 0; i< answer.size(); i++ )
        {
            r.insert(JSON_ASUINT(answer,i,0));
        }
    }
    return r;
}
std::set<uint32_t> Manager_Remote::passIndexesRequiredForLogin()
{
    json payload;
    std::set<uint32_t> r;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"passIndexesRequiredForLogin",payload,nullptr), jNull;

    if (jNull != answer)
    {
        for ( uint32_t i = 0; i< answer.size(); i++ )
        {
            r.insert(JSON_ASUINT(answer,i,0));
        }
    }
    return r;
}

std::string Manager_Remote::passIndexDescription(const uint32_t & passIndex)
{
    json payload;
    Secret_PublicData r;

    payload["passIndex" ] = passIndex;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"passIndexDescription",payload,nullptr), jNull;

    if (jNull == answer) return "";
    return answer.asString();
}

time_t Manager_Remote::accountExpirationDate(const std::string & sAccountName)
{
    json payload;
    Secret_PublicData r;

    payload["accountName" ] = sAccountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountExpirationDate",payload,nullptr), jNull;

    if (jNull == answer) return 0;
    return answer.asUInt64();
}

bool Manager_Remote::passIndexLoginRequired(const uint32_t &passIndex )
{
    json payload;
    Secret_PublicData r;

    payload["passIndex" ] = passIndex;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"passIndexLoginRequired",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return answer.asBool();
}

Secret_PublicData Manager_Remote::getAccountSecretPublicData(const std::string &sAccountName, uint32_t passIndex)
{
    json payload;
    Secret_PublicData r;

    payload["accountName" ] = sAccountName;
    payload["passIndex" ] = passIndex;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"getAccountSecretPublicData",payload,nullptr), jNull;

    if (jNull != answer)
    {
        std::map<std::string,std::string> ansmap;
        for ( auto i : answer.getMemberNames() )
        {
            ansmap[i] = JSON_ASSTRING(answer,i,"");
        }
        r.fromMap(ansmap);
    }
    return r;
}

bool Manager_Remote::isAccountSuperUser(const std::string &sAccountName)
{
    json payload;

    payload["accountName" ] = sAccountName;
    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"isAccountSuperUser",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

bool Manager_Remote::validateAccountAttribute(const std::string &sAccountName, const ApplicationAttribute &applicationAttrib)
{
    json payload;

    payload["attribName" ] = applicationAttrib.attribName;
    payload["accountName"] = sAccountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"validateAccountAttribute",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

bool Manager_Remote::attribAdd(const ApplicationAttribute &attrib, const std::string &attribDescription)
{
    json payload;

    payload["attribName" ] = attrib.attribName;
    payload["attribDescription"] = attribDescription;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"attribAdd",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

bool Manager_Remote::attribRemove(const ApplicationAttribute &attrib)
{
    json payload;

    payload["attribName" ] = attrib.attribName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"attribRemove",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

bool Manager_Remote::attribExist(const ApplicationAttribute &attrib)
{
    json payload;

    payload["attribName" ] = attrib.attribName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"attribExist",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

bool Manager_Remote::attribChangeDescription(const ApplicationAttribute &attrib, const std::string &attribDescription)
{
    json payload;

    payload["attribName" ] = attrib.attribName;
    payload["attribDescription"] = attribDescription;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"attribChangeDescription",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

std::string Manager_Remote::attribDescription(const ApplicationAttribute &attrib)
{
    json payload;

    payload["attribName" ] = attrib.attribName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"attribExist",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"attribDescription","");
}

json Manager_Remote::getStaticContent()
{
    // Get Static Content from the remote Authenticator Server. (scripts and resources)
    json payload;
    return m_fastRPC->runRemoteRPCMethod(CKEY,"getStaticContent",payload,nullptr);
}

int Manager_Remote::processFastRPCConnection(Mantids29::Network::Sockets::Socket_Stream_Base *stream)
{
    return m_fastRPC->processConnection(stream,CKEY);
}

sAccountAttribs Manager_Remote::accountAttribs(const std::string & accountName)
{
    json payload;

    sAccountAttribs attribs;

    payload["accountName"] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountAttribs",payload,nullptr), jNull;

    if (jNull == answer) return attribs;

    attribs.confirmed = JSON_ASBOOL(answer,"confirmed",false);
    attribs.enabled = JSON_ASBOOL(answer,"enabled",false);
    attribs.superuser = JSON_ASBOOL(answer,"superuser",false);

    return attribs;
}

std::string Manager_Remote::accountGivenName(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountGivenName",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"givenName","");
}

std::string Manager_Remote::accountLastName(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountLastName",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"lastName","");
}

std::string Manager_Remote::accountDescription(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountDescription",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"description","");
}

std::string Manager_Remote::accountEmail(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountEmail",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"email","");
}

std::string Manager_Remote::accountExtraData(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"accountExtraData",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"extraData","");
}

std::string Manager_Remote::applicationDescription(const std::string & applicationName)
{
    json payload;

    payload["applicationName" ] = applicationName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"applicationDescription",payload,nullptr), jNull;

    if (jNull == answer) return "";

    return JSON_ASSTRING(answer,"description","");
}

bool Manager_Remote::applicationValidateOwner(const std::string & applicationName, const std::string & accountName)
{
    json payload;

    payload["applicationName" ] = applicationName;
    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"applicationValidateOwner",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}
bool Manager_Remote::applicationValidateAccount(const std::string & applicationName, const std::string & accountName)
{
    json payload;

    payload["applicationName" ] = applicationName;
    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"applicationValidateAccount",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

std::set<std::string> Manager_Remote::applicationOwners(const std::string & applicationName)
{
    json payload;
    std::set<std::string> r;
    payload["applicationName" ] = applicationName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"applicationOwners",payload,nullptr), jNull;

    if (jNull != answer)
    {
        for ( uint32_t i = 0; i< answer.size(); i++ )
        {
            r.insert(JSON_ASSTRING(answer,i,""));
        }
    }
    return r;
}
std::set<std::string> Manager_Remote::applicationAccounts(const std::string & applicationName)
{
    json payload;
    std::set<std::string> r;
    payload["applicationName" ] = applicationName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"applicationAccounts",payload,nullptr), jNull;

    if (jNull != answer)
    {
        for ( uint32_t i = 0; i< answer.size(); i++ )
        {
            r.insert(JSON_ASSTRING(answer,i,""));
        }
    }
    return r;
}

bool Manager_Remote::isAccountDisabled(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"isAccountDisabled",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",true);
}

bool Manager_Remote::isAccountConfirmed(const std::string & accountName)
{
    json payload;

    payload["accountName" ] = accountName;

    json answer = m_fastRPC->runRemoteRPCMethod(CKEY,"isAccountConfirmed",payload,nullptr), jNull;

    if (jNull == answer) return false;

    return JSON_ASBOOL(answer,"retCode",false);
}

