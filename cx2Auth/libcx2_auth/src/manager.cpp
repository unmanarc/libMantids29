#include "manager.h"

#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_hlp_functions/random.h>

using namespace CX2::Authentication;


Manager::Manager()
{
}

Manager::~Manager()
{
}

bool Manager::initAccounts()
{
    // create the default account.
    Secret passData;
    passData.forceExpiration = true;
    passData.hash = CX2::Helpers::Random::createRandomString(16);

    return accountAdd("admin",  // UserName
                      passData, // Secret Info
                      "", // Email
                      "Autogenerated Superuser Account", // Account Description
                      "",  // Extra Data
                      0, // Expiration (don't expire)
                      true, // enabled
                      true, // confirmed
                      true // superuser
                      );
}

Reason Manager::authenticate(const std::string &accountName, const std::string &password, uint32_t passIndex, Mode authMode, const std::string &challengeSalt)
{
    Reason ret;
    bool found=false;
    Threads::Sync::Lock_RD lock(mutex);
    Secret pData = retrieveSecret(accountName,passIndex, &found);
    if (found)
    {
        if (!isAccountConfirmed(accountName)) ret = REASON_UNCONFIRMED_ACCOUNT;
        else if (isAccountDisabled(accountName)) ret = REASON_DISABLED_ACCOUNT;
        else if (isAccountExpired(accountName)) ret = REASON_EXPIRED_ACCOUNT;
        else ret = validateSecret(pData, password,  challengeSalt, authMode);
    }
    else
        ret = REASON_BAD_ACCOUNT;
    return ret;
}

std::string Manager::genRandomConfirmationToken()
{
    return CX2::Helpers::Random::createRandomString(64);
}

Secret_PublicData Manager::accountSecretPublicData(const std::string &accountName, bool *found, uint32_t passIndex)
{
    // protective-limited method.
    Secret pd = retrieveSecret(accountName, passIndex, found);
    Secret_PublicData pdb;

    if (!*found) pdb.passwordFunction = FN_NOTFOUND;
    else
    {
        pdb = pd.getBasicData();
    }

    return pdb;
}

bool Manager::accountChangeAuthenticatedSecret(const std::string &accountName, const std::string &currentPassword, Mode authMode, const std::string &challengeSalt, const Secret &passwordData, uint32_t passIndex)
{
    if (authenticate(accountName,currentPassword,passIndex,authMode,challengeSalt)!=REASON_AUTHENTICATED)
        return false;
    return accountChangeSecret(accountName,passwordData,passIndex);
}

bool Manager::isAccountExpired(const std::string &accountName)
{
    time_t tAccountExpirationDate = accountExpirationDate(accountName);
//    if (!tAccountExpirationDate) return false;
    return tAccountExpirationDate<time(nullptr);
}

std::set<std::string> Manager::accountUsableAttribs(const std::string &accountName)
{
    std::set<std::string> x;
    Threads::Sync::Lock_RD lock(mutex);
    // Take attribs from the account
    for (const std::string & attrib : accountDirectAttribs(accountName,false))
        x.insert(attrib);

    // Take the attribs from the belonging groups
    for (const std::string & groupName : accountGroups(accountName,false))
    {
        for (const std::string & attrib : groupAttribs(groupName,false))
            x.insert(attrib);
    }
    return x;
}

bool Manager::superUserAccountExist()
{
    auto accounts = accountsList();
    for (const std::string & account : accounts)
    {
        if (isAccountSuperUser(account))
            return true;
    }
    return false;
}

bool Manager::accountValidateAttribute(const std::string &accountName, const std::string &attribName)
{
    Threads::Sync::Lock_RD lock(mutex);
    if (accountValidateDirectAttribute(accountName,attribName))
    {
        return true;
    }
    for (const std::string & groupName : accountGroups(accountName,false))
    {
        if (groupValidateAttribute(groupName, attribName,false))
        {
            return true;
        }
    }
    return false;
}
