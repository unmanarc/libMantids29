#ifndef SQLCONNECTOR_H
#define SQLCONNECTOR_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <queue>
#include <set>

#include "authdata.h"
#include "query.h"

namespace CX2 { namespace Database {

class SQLConnector
{
public:
    SQLConnector();
    ~SQLConnector();


    // Database connection:

    bool connect( const std::string &dbFilePath );

    bool connect( const std::string &host,
                          const uint16_t & port,
                          const AuthData & auth,
                          const std::string & dbName
                          );

    virtual bool isOpen() = 0;

    // Database Internals:
    virtual bool dbTableExist(const std::string & table);


    // Connection/Database Information:
    virtual std::string driverName() = 0;

    std::string getLastSQLError() const;

    std::queue<std::string> getErrorsAndFlush();

    std::string getDBHostname() const;

    AuthData getDBAuthenticationData() const;

    AuthData getDBFullAuthenticationData() const;

    uint16_t getDBPort() const;

    std::string getDBFilePath() const;

    std::string getDBName() const;


    // SQL Query:
    Query * prepareNewQuery();

    void detachQuery( Query * query );


    // Fast Queries Approach:
    /**
     * @brief query Fast Prepared Query for non-return statements (non-select).
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query.
     * @return true if succeed.
     */
    bool query( std::string & preparedQuery, const std::map<std::string,Memory::Abstract::Var> & inputVars );
    /**
     * @brief query Fast Prepared Query for row-returning statements. (select)
     * @param preparedQuery Prepared SQL Query String.
     * @param inputVars Input Vars for the prepared query.
     * @param outputVars Output Vars for the step iteration.
     * @return pair of bool and query pointer
     *         if the query suceeed, the boolean will be true and there will be a query pointer.
     *         if the query can't be created, the boolean will be false and the query pointer nullptr.
     *         if the query was created, but can not be executed, the boolean is false, but the query is a valid pointer.
     *         NOTE: when the query is a valid pointer, you should delete/destroy the query.
     */
    std::pair<bool, Query *> query( std::string & preparedQuery,
                const std::map<std::string,Memory::Abstract::Var> & inputVars,
                const std::list<Memory::Abstract::Var *> & resultVars
                );


protected:
    virtual Query * createQuery0() { return nullptr; };
    virtual bool connect0() { return false; }

    std::string dbFilePath;
    std::string host;
    std::string dbName;
    uint16_t port;
    AuthData auth;

    std::string lastSQLError;

private:
    bool attachQuery( Query * query );

    std::set<Query *> querySet;
    bool finalized;
    std::mutex mtQuerySet;
    std::condition_variable cvEmptyQuerySet;
};

}}

#endif // SQLCONNECTOR_H
