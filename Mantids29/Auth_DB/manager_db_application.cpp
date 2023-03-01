#include "manager_db.h"

#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_datetime.h>
#include <Mantids29/Memory/a_bool.h>
#include <Mantids29/Memory/a_int32.h>
#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_var.h>
#include <Mantids29/Memory/a_uint64.h>

using namespace Mantids29::Authentication;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;
using namespace Mantids29::Helpers;

bool Manager_DB::applicationAdd(const std::string &appName, const std::string &applicationDescription, const std::string & sAppKey, const std::string & sOwnerAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v3_applications (`appName`,`f_appCreator`,`appDescription`,`appKey`) VALUES(:appName,:appCreator,:description,:appKey);",
                               {
                                   {":appName",new Abstract::STRING(appName)},
                                   {":appCreator",new Abstract::STRING(sOwnerAccountName)},
                                   {":description",new Abstract::STRING(applicationDescription)},
                                   {":appKey",new Abstract::STRING( Encoders::toBase64Obf(sAppKey) )}
                               });
}

bool Manager_DB::applicationRemove(const std::string &appName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("DELETE FROM vauth_v3_applications WHERE `appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(appName)}
                               });
}

bool Manager_DB::applicationExist(const std::string &appName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appDescription` FROM vauth_v3_applications WHERE `appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(appName)}
                                          },
                                          { });
    if (i->getResultsOK() && i->query->step())
    {
        ret = true;
    }
    return ret;
}

std::string Manager_DB::applicationDescription(const std::string &appName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appDescription` FROM vauth_v3_applications WHERE `appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(appName)}
                                          },
                                          { &description });
    if (i->getResultsOK() && i->query->step())
    {
        return description.getValue();
    }
    return "";
}

std::string Manager_DB::applicationKey(const std::string &appName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING appKey;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appKey` FROM vauth_v3_applications WHERE `appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(appName)}
                                          },
                                          { &appKey });
    if (i->getResultsOK() && i->query->step())
    {
        return Encoders::fromBase64Obf(appKey.getValue());
    }
    return "";
}

bool Manager_DB::applicationChangeKey(const std::string &appName, const std::string &appKey)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v3_applications SET `appKey`=:appKey WHERE `appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(appName)},
                                   {":appKey",new Abstract::STRING( Abstract::STRING( Encoders::toBase64Obf(appKey)) )}
                               });
}

bool Manager_DB::applicationChangeDescription(const std::string &appName, const std::string &applicationDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v3_applications SET `appDescription`=:description WHERE `appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(appName)},
                                   {":description",new Abstract::STRING(applicationDescription)}
                               });
}

std::set<std::string> Manager_DB::applicationList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAppName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appName` FROM vauth_v3_applications;",
                                          {},
                                          { &sAppName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(sAppName.getValue());
    }
    return ret;
}

bool Manager_DB::applicationValidateOwner(const std::string &appName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_applicationManaged` FROM vauth_v3_applicationmanagers WHERE `f_userNameManager`=:userName AND `f_applicationManaged`=:appName;",
                                          { {":appName",new Abstract::STRING(appName)},
                                            {":userName",new Memory::Abstract::STRING(sAccountName)}
                                          },
                                          { });
    return (i->getResultsOK() && i->query->step());
}

bool Manager_DB::applicationValidateAccount(const std::string &appName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_appName` FROM vauth_v3_applicationusers WHERE `f_userName`=:userName AND `f_appName`=:appName;",
                                          { {":appName",new Abstract::STRING(appName)},
                                            {":userName",new Memory::Abstract::STRING(sAccountName)}
                                          },
                                          { });
    return (i->getResultsOK() && i->query->step());
}

std::set<std::string> Manager_DB::applicationOwners(const std::string &appName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAccountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_userNameManager` FROM vauth_v3_applicationmanagers WHERE `f_applicationManaged`=:appName;",
                                          {
                                              {":appName",new Abstract::STRING(appName)}
                                          },
                                          { &sAccountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(sAccountName.getValue());
    }

    return ret;
}

std::set<std::string> Manager_DB::applicationAccounts(const std::string &appName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAccountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v3_applicationusers WHERE `f_appName`=:appName;",
                                          {
                                              {":appName",new Abstract::STRING(appName)}
                                          },
                                          { &sAccountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(sAccountName.getValue());
    }

    return ret;
}

std::set<std::string> Manager_DB::accountApplications(const std::string &sAccountName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sApplicationName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_appName` FROM vauth_v3_applicationusers WHERE `f_userName`=:userName;",
                                          {
                                              {":userName",new Abstract::STRING(sAccountName)}
                                          },
                                          { &sApplicationName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(sApplicationName.getValue());
    }

    return ret;
}

bool Manager_DB::applicationAccountAdd(const std::string &appName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v3_applicationusers (`f_userName`,`f_appName`) VALUES(:userName,:appName);",
                               {
                                   {":appName",new Abstract::STRING(appName)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::applicationAccountRemove(const std::string &appName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    bool ret = false;
    ret = m_sqlConnector->query("DELETE FROM vauth_v3_applicationusers WHERE `f_appName`=:appName AND `f_userName`=:userName;",
                              {
                                  {":appName",new Abstract::STRING(appName)},
                                  {":userName",new Abstract::STRING(sAccountName)}
                              });
    return ret;
}

bool Manager_DB::applicationOwnerAdd(const std::string &appName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v3_applicationmanagers (`f_userNameManager`,`f_applicationManaged`) VALUES(:userName,:appName);",
                               {
                                   {":appName",new Abstract::STRING(appName)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::applicationOwnerRemove(const std::string &appName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    bool ret = false;
    ret = m_sqlConnector->query("DELETE FROM vauth_v3_applicationmanagers WHERE `f_applicationManaged`=:appName AND `f_userNameManager`=:userName;",
                              {
                                  {":appName",new Abstract::STRING(appName)},
                                  {":userName",new Abstract::STRING(sAccountName)}
                              });
    return ret;
}

std::list<sApplicationSimpleDetails> Manager_DB::applicationsBasicInfoSearch(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<sApplicationSimpleDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sApplicationName,sAppCreator,description;

    std::string sSqlQuery = "SELECT `appName`,`f_appCreator`,`appDescription` FROM vauth_v3_applications";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" WHERE (`appName` LIKE :SEARCHWORDS OR `appDescription` LIKE :SEARCHWORDS)";
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
                                          { &sApplicationName, &sAppCreator, &description });
    while (i->getResultsOK() && i->query->step())
    {
        sApplicationSimpleDetails rDetail;

        rDetail.sAppCreator = sAppCreator.getValue();
        rDetail.sDescription = description.getValue();
        rDetail.sApplicationName = sApplicationName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}
