#include "query.h"
#include "sqlconnector.h"
#include <stdexcept>

using namespace CX2::Database;

Query::Query()
{
    mtDatabaseLock = nullptr;
    lastInsertRowID = 0;
    lastSQLReturnValue = 0;
    bFetchLastInsertRowID = true;
    sqlConnector = nullptr;
    bBindInputVars = false;
    bBindResultVars = false;
}

Query::~Query()
{
    if (sqlConnector)
    {
        ((SQLConnector *)sqlConnector)->detachQuery(this);
    }
    // Destroy strings.
    for (auto * i : destroyableStrings) delete i;

}

bool Query::setPreparedSQLQuery(const std::string &value, const std::map<std::string, CX2::Memory::Abstract::Var> &vars)
{
    query = value;

    if (!bindInputVars(vars))
        return false;

    return true;
}

bool Query::bindInputVars(const std::map<std::string, CX2::Memory::Abstract::Var> &vars)
{
    if (vars.empty()) return true;
    if (bBindInputVars)
        throw std::runtime_error("Don't call bindInputVars twice.");
    bBindInputVars = true;
    InputVars = vars;
    return postBindInputVars();
}

bool Query::bindResultVars(const std::vector<CX2::Memory::Abstract::Var *> &vars)
{
    if (vars.empty()) return true;
    if (bBindResultVars)
        throw std::runtime_error("Don't call bindResultVars twice.");
    bBindResultVars = true;
    resultVars = vars;
    return postBindResultVars();
}

std::string Query::getLastSQLError() const
{
    return lastSQLError;
}

int Query::getLastSQLReturnValue() const
{
    return lastSQLReturnValue;
}

unsigned long long Query::getLastInsertRowID() const
{
    return lastInsertRowID;
}

bool Query::getFetchLastInsertRowID() const
{
    return bFetchLastInsertRowID;
}

void Query::setFetchLastInsertRowID(bool value)
{
    bFetchLastInsertRowID = value;
}

bool Query::getIsNull(const size_t &column)
{
    if ((column+1) > isNull.size())
    {
        return true;
    }
    return isNull[column];
}

bool Query::replaceFirstKey(std::string &sqlQuery, std::list<std::string> &keysIn, std::vector<std::string> &keysOutByPos, const std::string replaceBy)
{
    std::list<std::string> toDelete;

    // Check who is the first key.
    std::size_t firstKeyPos = std::string::npos;
    std::string firstKeyFound;

    for ( auto & key : keysIn )
    {
        std::size_t pos = sqlQuery.find(key);
        if (pos!=std::string::npos)
        {
            if (pos <= firstKeyPos)
            {
                firstKeyPos = pos;
                firstKeyFound = key;
            }
        }
        else
            toDelete.push_back( key );
    }

    // not used needles will be deleted.
    for ( auto & needle : toDelete )
        keysIn.remove(needle);

    // If there is a key, replace.
    if (firstKeyPos!=std::string::npos)
    {
        keysOutByPos.push_back(firstKeyFound);
        sqlQuery.replace(firstKeyPos, firstKeyFound.length(), replaceBy);
        return true;
    }
    return false;
}

std::string *Query::createDestroyableString(const std::string &str)
{
    std::string * i = new std::string;
    *i = str;
    destroyableStrings.push_back(i);
    return i;
}

void Query::setSqlConnector(void *value, std::mutex *mtDatabaseLock)
{
    this->mtDatabaseLock = mtDatabaseLock;
    sqlConnector = value;
}
