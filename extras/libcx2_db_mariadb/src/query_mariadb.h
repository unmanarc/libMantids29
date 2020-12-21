#ifndef QUERY_MARIADB_H
#define QUERY_MARIADB_H

#include <cx2_db/query.h>
#include <mysql/mysql.h>
#include <vector>

namespace CX2 { namespace Database {

class Query_MariaDB : public Query
{
public:
    Query_MariaDB();
    ~Query_MariaDB();

    // Direct Query:
    bool exec(const ExecType & execType);
    bool step();

    // MariaDB specific functions:
    bool mariadbInitSTMT( MYSQL *dbCnt );

    bool getFetchInsertRowID() const;
    void setFetchInsertRowID(bool value);

    my_ulonglong getLastInsertRowID() const;

private:
    bool replaceFirstKey(std::string & sqlQuery, std::list<std::string> & keysIn, std::vector<std::string> &keysOutByPos );

    bool postBindInputVars();
    bool postBindResultVars();

    std::string * createDestroyableString(const std::string & str);

    MYSQL_STMT * stmt;
    MYSQL_BIND * bindedParams;
    MYSQL_BIND * bindedResults;
    my_bool * bIsNull;

    bool bDeallocateResultSet;
    bool bFetchInsertRowID;
    my_ulonglong lastInsertRowID;

    std::vector<std::string> keysByPos;
    std::list<std::string *> destroyableStrings;

};
}}

#endif // QUERY_MARIADB_H