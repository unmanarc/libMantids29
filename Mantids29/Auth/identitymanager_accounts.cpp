#include "identitymanager.h"

#include <Mantids29/Threads/lock_shared.h>
#include <Mantids29/Helpers/random.h>

using namespace Mantids29::Auth;

IdentityManager::Users::Users(IdentityManager *m_parent) { this->m_parent = m_parent;}

bool IdentityManager::Users::isAccountExpired(const std::string &accountName)
{
    time_t tAccountExpirationDate = getAccountExpirationDate(accountName);
    if (!tAccountExpirationDate) return false;
    return tAccountExpirationDate<time(nullptr);
}



bool IdentityManager::Users::hasSuperUserAccount()
{
    auto accounts = listAccounts();
    for (const std::string & account : accounts)
    {
        if (getAccountFlags(account).superuser)
            return true;
    }
    return false;
}
