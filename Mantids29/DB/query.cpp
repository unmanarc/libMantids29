#include "query.h"
#include "sqlconnector.h"
#include <stdexcept>

using namespace Mantids29::Database;

Query::~Query()
{
    if (m_pSQLConnector)
    {
        // Detach me from the SQL connector...
        ((SQLConnector *)m_pSQLConnector)->detachQuery(this);
    }

    // Destroy Input Vars...
    for (auto & i : m_inputVars)
        delete i.second;

    m_inputVars.clear();

    clearDestroyableStringsForInput();
    clearDestroyableStringsForResults();

    if (m_databaseLockMutex)
        m_databaseLockMutex->unlock();
}

bool Query::setPreparedSQLQuery(const std::string &value, const std::map<std::string, Memory::Abstract::Var *> &vars)
{
    m_query = value;

    if (!bindInputVars(vars))
        return false;

    return true;
}

bool Query::bindInputVars(const std::map<std::string, Mantids29::Memory::Abstract::Var *> &vars)
{
    if (vars.empty()) return true;
    if (m_bindInputVars)
        throw std::runtime_error("Don't call bindInputVars twice.");
    m_bindInputVars = true;
    m_inputVars = vars;
    return postBindInputVars();
}

bool Query::bindResultVars(const std::vector<Mantids29::Memory::Abstract::Var *> &vars)
{
    if (vars.empty()) return true;
    if (m_bindResultVars)
        throw std::runtime_error("Don't call bindResultVars twice.");
    m_bindResultVars = true;
    m_resultVars = vars;
    return postBindResultVars();
}

std::string Query::getLastSQLError() const
{
    return m_lastSQLError;
}

int Query::getLastSQLReturnValue() const
{
    return m_lastSQLReturnValue;
}

unsigned long long Query::getLastInsertRowID() const
{
    return m_lastInsertRowID;
}

bool Query::exec(const ExecType &execType)
{
    return exec0(execType,false);
}

bool Query::step()
{
    clearDestroyableStringsForResults();
    m_fieldIsNull.clear();
    return step0();
}

bool Query::isNull(const size_t &column)
{
    if ((column+1) > m_fieldIsNull.size())
    {
        return true;
    }
    return m_fieldIsNull[column];
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

std::string *Query::createDestroyableStringForInput(const std::string &str)
{
    std::string * i = new std::string;
    *i = str;
    m_destroyableStringsForInput.push_back(i);
    return i;
}

void Query::clearDestroyableStringsForInput()
{
    // Destroy strings.
    for (auto * i : m_destroyableStringsForInput) delete i;
}

std::string *Query::createDestroyableStringForResults(const std::string &str)
{
    std::string * i = new std::string;
    *i = str;
    m_destroyableStringsForResults.push_back(i);
    return i;
}

void Query::clearDestroyableStringsForResults()
{
    // Destroy strings.
    for (auto * i : m_destroyableStringsForResults) delete i;
    m_destroyableStringsForResults.clear();
}

uint64_t Query::getAffectedRows() const
{
    return m_affectedRows;
}

uint64_t Query::getNumRows() const
{
    return m_numRows;
}

bool Query::setSqlConnector(void *value, std::timed_mutex *m_databaseLockMutex, const uint64_t &milliseconds)
{
    this->m_databaseLockMutex = m_databaseLockMutex;
    m_pSQLConnector = value;

    // Adquire the lock here (some DB's can only handle one query at time)
    if (milliseconds == 0)
        m_databaseLockMutex->lock();
    else
    {
        if (m_databaseLockMutex->try_lock_for( std::chrono::milliseconds(milliseconds) ))
            return true;
        else
        {
            // No need to unlock at the end...
            this->m_databaseLockMutex = nullptr;
            return false;
        }
    }
    return true;
}
