#include "manager_db.h"

#include <limits>
#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_datetime.h>
#include <Mantids29/Memory/a_bool.h>
#include <Mantids29/Memory/a_int32.h>
#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_uint64.h>
#include <Mantids29/Memory/a_var.h>

using namespace Mantids29::Authentication;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;

bool Manager_DB::accountAdd(const std::string &accountName, const Secret &secretData, const AccountDetailsWExtraData &accountDetails, time_t expirationDate, const AccountBasicAttributes &accountAttribs, const std::string &sCreatorAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    return m_sqlConnector->query("INSERT INTO vauth_v4_accounts (`userName`,`givenName`,`lastName`,`email`,`description`,`extraData`,`isSuperuser`,`isEnabled`,`expiration`,`isConfirmed`,`creator`) "
                                                       "VALUES(:userName ,:givenname ,:lastname ,:email ,:description ,:extraData ,:superuser ,:enabled ,:expiration ,:confirmed ,:creator);",
                               {
                                   {":userName",new Abstract::STRING(accountName)},
                                   {":givenname",new Abstract::STRING(accountDetails.givenName)},
                                   {":lastname",new Abstract::STRING(accountDetails.lastName)},
                                   {":email",new Abstract::STRING(accountDetails.email)},
                                   {":description",new Abstract::STRING(accountDetails.description)},
                                   {":extraData",new Abstract::STRING(accountDetails.extraData)},
                                   {":superuser",new Abstract::BOOL(accountAttribs.superuser)},
                                   {":enabled",new Abstract::BOOL(accountAttribs.enabled)},
                                   {":expiration",new Abstract::DATETIME(expirationDate)},
                                   {":confirmed",new Abstract::BOOL(accountAttribs.confirmed)},
                                   {":creator", sCreatorAccountName.empty() ? new Abstract::Var() /* null */ : new Abstract::STRING(accountDetails.extraData)}
                               }
                               )
            &&
            m_sqlConnector->query("INSERT INTO vauth_v4_accountactivationtokens (`f_userName`,`confirmationToken`) "
                                "VALUES(:account,:confirmationToken);",
                                {
                                    {":account",new Abstract::STRING(accountName)},
                                    {":confirmationToken",new Abstract::STRING(genRandomConfirmationToken())}
                                }
                                )
            &&
            m_sqlConnector->query("INSERT INTO vauth_v4_accountsecrets "
                                "(`f_secretIndex`,`f_userName`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`)"
                                " VALUES"
                                "('0',:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                                {
                                    {":account",new Abstract::STRING(accountName)},
                                    {":hash",new Abstract::STRING(secretData.hash)},
                                    {":expiration",new Abstract::DATETIME(secretData.expirationTimestamp)},
                                    {":function",new Abstract::INT32(secretData.passwordFunction)},
                                    {":salt",new Abstract::STRING(Mantids29::Helpers::Encoders::toHex(secretData.ssalt,4))},
                                    {":forcedExpiration",new Abstract::BOOL(secretData.forceExpiration)},
                                    {":steps",new Abstract::UINT32(secretData.gAuthSteps)}
                                }
                                );

}

std::string Manager_DB::getAccountConfirmationToken(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING token;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT confirmationToken FROM vauth_v4_accountactivationtokens WHERE `f_userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &token });
    if (i->getResultsOK() && i->query->step())
    {
        return token.getValue();
    }
    return "";
}

bool Manager_DB::accountRemove(const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (isThereAnotherSuperUser(accountName))
    {
        return m_sqlConnector->query("DELETE FROM vauth_v4_accounts WHERE `userName`=:userName;",
                                   {
                                       {":userName",new Abstract::STRING(accountName)}
                                   });
    }
    return false;
}

bool Manager_DB::accountExist(const std::string &accountName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isEnabled` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          {{":userName",new Memory::Abstract::STRING(accountName)}},
                                          { });
    if (i->getResultsOK() && i->query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::accountDisable(const std::string &accountName, bool disabled)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (disabled==true && !isThereAnotherSuperUser(accountName))
    {
        return false;
    }

    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `isEnabled`=:enabled WHERE `userName`=:userName;",
                               {
                                   {":enabled",new Abstract::BOOL(!disabled)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountConfirm(const std::string &accountName, const std::string &confirmationToken)
{
    Threads::Sync::Lock_RW lock(mutex);

    Abstract::STRING token;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `confirmationToken` FROM vauth_v4_accountactivationtokens WHERE `f_userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &token });

    if (i->getResultsOK() && i->query->step())
    {
        if (!token.getValue().empty() && token.getValue() == confirmationToken)
        {
            return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `isConfirmed`='1' WHERE `userName`=:userName;",
                                       {
                                           {":userName",new Abstract::STRING(accountName)}
                                       });
        }
    }
    return false;
}

bool Manager_DB::accountChangeSecret(const std::string &accountName, const Secret &passwordData, uint32_t passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);

    // Destroy (if exist).
    m_sqlConnector->query("DELETE FROM vauth_v4_accountsecrets WHERE `f_userName`=:userName and `f_secretIndex`=:index",
                        {
                            {":userName",new Abstract::STRING(accountName)},
                            {":index",new Abstract::UINT32(passIndex)}
                        });

    return m_sqlConnector->query("INSERT INTO vauth_v4_accountsecrets "
                               "(`f_secretIndex`,`f_userName`,`hash`,`expiration`,`function`,`salt`,`forcedExpiration`,`steps`) "
                               "VALUES"
                               "(:index,:account,:hash,:expiration,:function,:salt,:forcedExpiration,:steps);",
                               {
                                   {":index",new Abstract::UINT32(passIndex)},
                                   {":account",new Abstract::STRING(accountName)},
                                   {":hash",new Abstract::STRING(passwordData.hash)},
                                   {":expiration",new Abstract::DATETIME(passwordData.expirationTimestamp)},
                                   {":function",new Abstract::UINT32(passwordData.passwordFunction)},
                                   {":salt",new Abstract::STRING(Mantids29::Helpers::Encoders::toHex(passwordData.ssalt,4))},
                                   {":forcedExpiration",new Abstract::BOOL(passwordData.forceExpiration)},
                                   {":steps",new Abstract::UINT32(passwordData.gAuthSteps)}
                               });

}

bool Manager_DB::accountChangeDescription(const std::string &accountName, const std::string &description)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `description`=:description WHERE `userName`=:userName;",
                               {
                                   {":description",new Abstract::STRING(description)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeGivenName(const std::string &accountName, const std::string &givenName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `givenName`=:givenname WHERE `userName`=:userName;",
                               {
                                   {":givenname",new Abstract::STRING(givenName)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeLastName(const std::string &accountName, const std::string &lastName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `lastName`=:lastname WHERE `userName`=:userName;",
                               {
                                   {":lastname",new Abstract::STRING(lastName)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeEmail(const std::string &accountName, const std::string &email)
{
    Threads::Sync::Lock_RW lock(mutex);

    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `email`=:email WHERE `userName`=:userName;",
                               {
                                   {":email",new Abstract::STRING(email)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeExtraData(const std::string &accountName, const std::string &extraData)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `extraData`=:extraData WHERE `userName`=:userName;",
                               {
                                   {":extraData",new Abstract::STRING(extraData)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::accountChangeExpiration(const std::string &accountName, time_t expiration)
{
    Threads::Sync::Lock_RW lock(mutex);

    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `expiration`=:expiration WHERE `userName`=:userName;",
                               {
                                   {":expiration",new Abstract::DATETIME(expiration)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

AccountBasicAttributes Manager_DB::accountAttribs(const std::string &accountName)
{
    AccountBasicAttributes r;

    Abstract::BOOL enabled,confirmed,superuser;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isEnabled`,`isConfirmed`,`isSuperuser` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &enabled,&confirmed,&superuser});

    if (i->getResultsOK() && i->query->step())
    {
        r.enabled = enabled.getValue();
        r.confirmed = confirmed.getValue();
        r.superuser = superuser.getValue();
    }


    return r;
}

bool Manager_DB::accountChangeGroupSet(const std::string &accountName, const std::set<std::string> &groupSet)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (!m_sqlConnector->query("DELETE FROM vauth_v4_groupsaccounts WHERE `f_userName`=:userName;",
    {
        {":userName",new Abstract::STRING(accountName)}
    }))
        return false;

    for (const auto & group : groupSet)
    {
        if (!m_sqlConnector->query("INSERT INTO vauth_v4_groupsaccounts (`f_groupName`,`f_userName`) VALUES(:groupName,:userName);",
        {
            {":groupName",new Abstract::STRING(group)},
            {":userName",new Abstract::STRING(accountName)}
        }))
        return false;
    }
    return true;
}

bool Manager_DB::accountChangeAttribs(const std::string &accountName, const AccountBasicAttributes &accountAttribs)
{
    Threads::Sync::Lock_RW lock(mutex);

    if ((accountAttribs.confirmed==false || accountAttribs.enabled==false || accountAttribs.superuser==false) && !isThereAnotherSuperUser(accountName))
    {
        return false;
    }

    return m_sqlConnector->query("UPDATE vauth_v4_accounts SET `isEnabled`=:enabled,`isConfirmed`=:confirmed,`isSuperuser`=:superuser WHERE `userName`=:userName;",
                               {
                                   {":enabled",new Abstract::BOOL(accountAttribs.enabled)},
                                   {":confirmed",new Abstract::BOOL(accountAttribs.confirmed)},
                                   {":superuser",new Abstract::BOOL(accountAttribs.superuser)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::isAccountDisabled(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL enabled;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isEnabled` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &enabled });

    if (i->getResultsOK() && i->query->step())
    {
        return !enabled.getValue();
    }
    return true;
}

bool Manager_DB::isAccountConfirmed(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL confirmed;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isConfirmed` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &confirmed });

    if (i->getResultsOK() && i->query->step())
    {
        return confirmed.getValue();
    }
    return false;
}

bool Manager_DB::isAccountSuperUser(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::BOOL superuser;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isSuperuser` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &superuser });

    if (i->getResultsOK() && i->query->step())
    {
        return superuser.getValue();
    }
    return false;
}

std::string Manager_DB::accountGivenName(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING givenName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `givenName` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &givenName });

    if (i->getResultsOK() && i->query->step())
    {
        return givenName.getValue();
    }
    return "";
}

std::string Manager_DB::accountLastName(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING lastName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `lastName` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &lastName });

    if (i->getResultsOK() && i->query->step())
    {
        return lastName.getValue();
    }
    return "";
}

std::string Manager_DB::accountDescription(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `description` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &description });

    if (i->getResultsOK() && i->query->step())
    {
        return description.getValue();
    }
    return "";
}

std::string Manager_DB::accountEmail(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING email;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `email` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &email });

    if (i->getResultsOK() && i->query->step())
    {
        return email.getValue();
    }
    return "";
}

std::string Manager_DB::accountExtraData(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING extraData;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `extraData` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &extraData });

    if (i->getResultsOK() && i->query->step())
    {
        return extraData.getValue();
    }
    return "";
}

time_t Manager_DB::accountExpirationDate(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::DATETIME expiration;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `expiration` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &expiration });
    if (i->getResultsOK() && i->query->step())
    {
        return expiration.getValue();
    }
    // If can't get this data, the account is expired:
    return 1;
}

void Manager_DB::updateLastLogin(const std::string &accountName, const uint32_t &uPassIdx, const ClientDetails &clientDetails)
{
    Threads::Sync::Lock_RW lock(mutex);

    m_sqlConnector->query("UPDATE vauth_v4_accounts SET `lastLogin`=CURRENT_TIMESTAMP WHERE `userName`=:userName;",
                        {
                            {":userName",new Abstract::STRING(accountName)}
                        });

    m_sqlConnector->query("INSERT INTO vauth_v4_accountlogins(`f_userName`,`f_secretIndex`,`loginDateTime`,`loginIP`,`loginTLSCN`,`loginUserAgent`,`loginExtraData`) "
                        "VALUES "
                        "(:userName,:index,:date,:loginIP,:loginTLSCN,:loginUserAgent,:loginExtraData);",
                        {
                            {":userName",new Abstract::STRING(accountName)},
                            {":index",new Abstract::UINT32(uPassIdx)},
                            {":date",new Abstract::DATETIME(time(nullptr))},
                            {":loginIP",new Abstract::STRING(clientDetails.ipAddress)},
                            {":loginTLSCN",new Abstract::STRING(clientDetails.tlsCommonName)},
                            {":loginUserAgent",new Abstract::STRING(clientDetails.userAgent)},
                            {":loginExtraData",new Abstract::STRING(clientDetails.extraData)}
                        }
                        );

}

time_t Manager_DB::accountLastLogin(const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::DATETIME lastLogin;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `lastLogin` FROM vauth_v4_accounts WHERE `userName`=:userName LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &lastLogin });
    if (i->getResultsOK() && i->query->step())
    {
        return lastLogin.getValue();
    }
    // If can't get this data, the account is expired:
    return 1;
}

void Manager_DB::resetBadAttempts(const std::string &accountName, const uint32_t &passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    m_sqlConnector->query("UPDATE vauth_v4_accountsecrets SET `badAttempts`='0' WHERE `f_userName`=:userName and `f_secretIndex`=:index;",
                        {
                            {":userName",new Abstract::STRING(accountName)},
                            {":index",new Abstract::UINT32(passIndex)}
                        });

}

void Manager_DB::incrementBadAttempts(const std::string &accountName, const uint32_t &passIndex)
{
    Threads::Sync::Lock_RW lock(mutex);
    m_sqlConnector->query("UPDATE vauth_v4_accountsecrets SET `badAttempts`=`badAttempts`+1  WHERE `f_userName`=:userName and `f_secretIndex`=:index;",
                        {
                            {":userName",new Abstract::STRING(accountName)},
                            {":index",new Abstract::UINT32(passIndex)}
                        });
}

std::list<AccountDetails> Manager_DB::accountsBasicInfoSearch(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<AccountDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING accountName,givenName,lastName,email,description;
    Abstract::BOOL superuser,enabled,confirmed;
    Abstract::DATETIME expiration;

    std::string sSqlQuery = "SELECT `userName`,`givenName`,`lastName`,`email`,`description`,`isSuperuser`,`isEnabled`,`expiration`,`isConfirmed` FROM vauth_v4_accounts";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" WHERE (`userName` LIKE :SEARCHWORDS OR `givenName` LIKE :SEARCHWORDS OR `lastName` LIKE :SEARCHWORDS OR `email` LIKE :SEARCHWORDS OR `description` LIKE :SEARCHWORDS)";
    }

    if (limit)
        sSqlQuery+=" LIMIT :LIMIT OFFSET :OFFSET";

    sSqlQuery+=";";

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect(sSqlQuery,
                                          {
                                              {":SEARCHWORDS",new Abstract::STRING(sSearchWords)},
                                              {":LIMIT",new Abstract::UINT64(limit)},
                                              {":OFFSET",new Abstract::UINT64(offset)}
                                          },
                                          { &accountName, &givenName, &lastName, &email, &description, &superuser, &enabled, &expiration, &confirmed });
    while (i->getResultsOK() && i->query->step())
    {
        AccountDetails rDetail;

        rDetail.confirmed = confirmed.getValue();
        rDetail.enabled = enabled.getValue();
        rDetail.superuser = superuser.getValue();
        rDetail.givenName = givenName.getValue();
        rDetail.lastName = lastName.getValue();
        rDetail.description = description.getValue();
        rDetail.expired = !expiration.getValue()?false:expiration.getValue()<time(nullptr);
        rDetail.email = email.getValue();
        rDetail.accountName = accountName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

std::set<std::string> Manager_DB::accountsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `userName` FROM vauth_v4_accounts;",
                                          { },
                                          { &accountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(accountName.getValue());
    }

    return ret;
}

std::set<std::string> Manager_DB::accountGroups(const std::string &accountName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lockShared();

    Abstract::STRING group;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_groupName` FROM vauth_v4_groupsaccounts WHERE `f_userName`=:userName;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &group });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(group.getValue());
    }

    if (lock) mutex.unlockShared();
    return ret;
}

std::set<ApplicationAttribute> Manager_DB::accountDirectAttribs(const std::string &accountName, bool lock)
{
    std::set<ApplicationAttribute> ret;
    if (lock) mutex.lockShared();

    Abstract::STRING appName,attrib;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_appName`,`f_attribName` FROM vauth_v4_attribsaccounts WHERE `f_userName`=:userName;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)} },
                                          { &appName,&attrib });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert( { appName.getValue(), attrib.getValue() } );
    }

    if (lock) mutex.unlockShared();
    return ret;
}

bool Manager_DB::superUserAccountExist()
{
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isSuperuser` FROM vauth_v4_accounts WHERE `isSuperuser`=:superuser LIMIT 1;",
                                          { {":superuser",new Memory::Abstract::BOOL(true)} },
                                          { });

    if (i->getResultsOK() && i->query->step())
        return true;

    return false;
}


Secret Manager_DB::retrieveSecret(const std::string &accountName, uint32_t passIndex, bool *accountFound, bool *indexFound)
{
    Secret ret;
    *indexFound = false;
    *accountFound = false;

    Threads::Sync::Lock_RD lock(mutex);

    Abstract::UINT32 steps,function,badAttempts;
    Abstract::BOOL forcedExpiration;
    Abstract::DATETIME expiration;
    Abstract::STRING salt,hash;

    *accountFound = accountExist(accountName);

    if (!*accountFound)
        return ret;

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `steps`,`forcedExpiration`,`function`,`expiration`,`badAttempts`,`salt`,`hash` FROM vauth_v4_accountsecrets "
                                          "WHERE `f_userName`=:userName AND `f_secretIndex`=:index LIMIT 1;",
                                          { {":userName",new Memory::Abstract::STRING(accountName)},
                                            {":index",new Memory::Abstract::UINT32(passIndex)}
                                          },
                                          { &steps, &forcedExpiration, &function, &expiration, &badAttempts, &salt, &hash });

    if (i->getResultsOK() && i->query->step())
    {
        *indexFound = true;
        ret.gAuthSteps = steps.getValue();
        ret.forceExpiration = forcedExpiration.getValue();
        ret.passwordFunction = (Authentication::Function)function.getValue();
        ret.expirationTimestamp = expiration.getValue();
        ret.badAttempts = badAttempts.getValue();
        Mantids29::Helpers::Encoders::fromHex(salt.getValue(),ret.ssalt,4);
        ret.hash = hash.getValue();
    }
    return ret;
}

bool Manager_DB::isThereAnotherSuperUser(const std::string &accountName)
{
    // Check if there is any superuser acount beside this "to be deleted" account...
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `isEnabled` FROM vauth_v4_accounts WHERE `userName`!=:userName and `isSuperuser`=:superUser and enabled=:enabled LIMIT 1;",
                                          {
                                              {":userName",new Memory::Abstract::STRING(accountName)},
                                              {":superUser",new Memory::Abstract::BOOL(true)},
                                              {":enabled",new Memory::Abstract::BOOL(true)}
                                          },
                                          { });

    if (i->getResultsOK() && i->query->step())
        return true;
    return false;

}

