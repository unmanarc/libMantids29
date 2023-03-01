#ifndef MANAGER_DB_H
#define MANAGER_DB_H

#include <Mantids29/Auth/manager.h>
#include <Mantids29/DB/sqlconnector.h>

namespace Mantids29 { namespace Authentication {

class Manager_DB : public Manager
{
public:
    // Open authentication system:
    Manager_DB( Mantids29::Database::SQLConnector * sqlConnector );
    bool initScheme() override;

    /////////////////////////////////////////////////////////////////////////////////
    // Pass Indexes:
    std::set<uint32_t> passIndexesUsedByAccount(const std::string & sAccountName) override;
    std::set<uint32_t> passIndexesRequiredForLogin() override;
    bool passIndexAdd(const uint32_t & passIndex, const std::string & description, const bool & loginRequired) override;
    bool passIndexModify(const uint32_t & passIndex, const std::string & description, const bool & loginRequired) override;
    bool passIndexDelete(const uint32_t & passIndex) override;
    std::string passIndexDescription(const uint32_t & passIndex) override;
    bool passIndexLoginRequired(const uint32_t & passIndex) override;


    /////////////////////////////////////////////////////////////////////////////////
    // account:
    bool accountAdd(        const std::string & sAccountName,
                            const Secret &secretData,
                            const sAccountDetails & accountDetails = { "","","","","" },
                            time_t expirationDate = 0, // Note: use 1 to create an expired account.
                            const sAccountAttribs & accountAttribs = {true,true,false},
                            const std::string & sCreatorAccountName = "") override;

    std::string getAccountConfirmationToken(const std::string & sAccountName) override;
    bool accountRemove(const std::string & sAccountName) override;
    bool accountExist(const std::string & sAccountName) override;
    bool accountDisable(const std::string & sAccountName, bool disabled = true) override;
    bool accountConfirm(const std::string & sAccountName, const std::string & confirmationToken) override;
    bool accountChangeSecret(const std::string & sAccountName, const Secret & passwordData, uint32_t passIndex=0) override;
    bool accountChangeDescription(const std::string & sAccountName, const std::string & description) override;
    bool accountChangeGivenName(const std::string & sAccountName, const std::string & sGivenName) override;
    bool accountChangeLastName(const std::string & sAccountName, const std::string & sLastName) override;
    bool accountChangeEmail(const std::string & sAccountName, const std::string & email) override;
    bool accountChangeExtraData(const std::string & sAccountName, const std::string & extraData) override;
    bool accountChangeExpiration(const std::string & sAccountName, time_t expiration = 0) override;
    sAccountAttribs accountAttribs(const std::string & sAccountName) override;
    bool accountChangeGroupSet( const std::string & sAccountName, const std::set<std::string> & groupSet ) override;
    bool accountChangeAttribs(const std::string & sAccountName,const sAccountAttribs & accountAttribs) override;
    bool isAccountDisabled(const std::string & sAccountName) override;
    bool isAccountConfirmed(const std::string & sAccountName) override;
    bool isAccountSuperUser(const std::string & sAccountName) override;
    std::string accountGivenName(const std::string & sAccountName) override;
    std::string accountLastName(const std::string & sAccountName) override;
    std::string accountDescription(const std::string & sAccountName) override;
    std::string accountEmail(const std::string & sAccountName) override;
    std::string accountExtraData(const std::string & sAccountName) override;
    time_t accountExpirationDate(const std::string & sAccountName) override;

    void updateLastLogin(const std::string &sAccountName, const uint32_t & uPassIdx, const ClientDetails & clientDetails) override;

    time_t accountLastLogin(const std::string & sAccountName) override;
    void resetBadAttempts(const std::string & sAccountName, const uint32_t & passIndex) override;
    void incrementBadAttempts(const std::string & sAccountName, const uint32_t & passIndex) override;

    std::list<sAccountSimpleDetails> accountsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;
    std::set<std::string> accountsList() override;
    std::set<std::string> accountGroups(const std::string & sAccountName, bool lock = true) override;
    std::set<ApplicationAttribute> accountDirectAttribs(const std::string & sAccountName, bool lock = true) override;

    bool superUserAccountExist() override;

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    bool applicationAdd(const std::string & appName, const std::string & applicationDescription, const std::string &sAppKey, const std::string & sOwnerAccountName) override;
    bool applicationRemove(const std::string & appName) override;
    bool applicationExist(const std::string & appName) override;
    std::string applicationDescription(const std::string & appName) override;
    std::string applicationKey(const std::string & appName) override;
    bool applicationChangeKey(const std::string & appName, const std::string & appKey) override;
    bool applicationChangeDescription(const std::string & appName, const std::string & applicationDescription) override;
    std::set<std::string> applicationList() override;
    bool applicationValidateOwner(const std::string & appName, const std::string & sAccountName) override;
    bool applicationValidateAccount(const std::string & appName, const std::string & sAccountName) override;
    std::set<std::string> applicationOwners(const std::string & appName) override;
    std::set<std::string> applicationAccounts(const std::string & appName) override;
    std::set<std::string> accountApplications(const std::string & sAccountName) override;
    bool applicationAccountAdd(const std::string & appName, const std::string & sAccountName) override;
    bool applicationAccountRemove(const std::string & appName, const std::string & sAccountName) override;
    bool applicationOwnerAdd(const std::string & appName, const std::string & sAccountName) override;
    bool applicationOwnerRemove(const std::string & appName, const std::string & sAccountName) override;
    std::list<sApplicationSimpleDetails> applicationsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const ApplicationAttribute & applicationAttrib, const std::string & attribDescription) override;
    bool attribRemove(const ApplicationAttribute & applicationAttrib) override;
    bool attribExist(const ApplicationAttribute & applicationAttrib) override;
    bool attribGroupAdd(const ApplicationAttribute & applicationAttrib, const std::string & groupName) override;
    bool attribGroupRemove(const ApplicationAttribute & applicationAttrib, const std::string & groupName, bool lock = true) override;
    bool attribAccountAdd(const ApplicationAttribute & applicationAttrib, const std::string & sAccountName) override;
    bool attribAccountRemove(const ApplicationAttribute & applicationAttrib, const std::string & sAccountName, bool lock = true) override;
    bool attribChangeDescription(const ApplicationAttribute & applicationAttrib, const std::string & attribDescription) override;
    std::string attribDescription(const ApplicationAttribute & applicationAttrib) override;
    std::set<ApplicationAttribute> attribsList(const std::string & applicationName = "") override;
    std::set<std::string> attribGroups(const ApplicationAttribute & applicationAttrib, bool lock = true) override;
    std::set<std::string> attribAccounts(const ApplicationAttribute & applicationAttrib, bool lock = true) override;
    std::list<sAttributeSimpleDetails> attribsBasicInfoSearch(const std::string & appName, std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    bool groupAdd(const std::string & groupName, const std::string & groupDescription) override;
    bool groupRemove(const std::string & groupName) override;
    bool groupExist(const std::string & groupName) override;
    bool groupAccountAdd(const std::string & groupName, const std::string & sAccountName) override;
    bool groupAccountRemove(const std::string & groupName, const std::string & sAccountName, bool lock = true) override;
    bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription) override;
    bool groupValidateAttribute(const std::string & groupName, const ApplicationAttribute & attrib, bool lock =true) override;
    std::string groupDescription(const std::string & groupName) override;
    std::set<std::string> groupsList() override;
    std::set<ApplicationAttribute> groupAttribs(const std::string & groupName, bool lock = true) override;
    std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true) override;
    std::list<sGroupSimpleDetails> groupsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;

    std::list<std::string> getSqlErrorList() const;
    void clearSQLErrorList();

protected:
    bool accountValidateDirectAttribute(const std::string & sAccountName, const ApplicationAttribute & applicationAttrib) override;
    Secret retrieveSecret(const std::string &sAccountName, uint32_t passIndex, bool * accountFound, bool * indexFound) override;

private:

    bool isThereAnotherSuperUser(const std::string &sAccountName);

    std::list<std::string> m_sqlErrorList;
    Mantids29::Database::SQLConnector * m_sqlConnector;
};


}}
#endif // MANAGER_DB_H
