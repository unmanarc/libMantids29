#ifndef REQUEST_H
#define REQUEST_H

#include <mdz_hlp_functions/json.h>
#include <set>
//#include <mdz_mem_vars/subparser.h>
//#include "streamablejson.h"
#include "data.h"

namespace Mantids { namespace Authentication {

class Multi
{
public:
    Multi();

    /**
     * @brief print Print the request to console
     */
  // void print();
    /**
     * @brief setAuthentications Set the authentication string.
     * @param sAuthentications string in JSON Format.
     * @return if the string have been correctly parsed, returns true, else false.
     */
    bool setAuthentications(const std::string & sAuthentications);

    bool setJSON(const json & auths);

    /**
     * @brief clear Clear authentications
     */
    void clear();
    /**
     * @brief addAuthentication Manually add an authentication
     * @param auth Authentication object.
     */
    void addAuthentication(const Data & auth);
    /**
     * @brief addAuthentication Add an authentication as passIndex+Secret
     * @param passIndex Authentication Secret Index
     * @param pass Secret
     */
    void addAuthentication(uint32_t passIndex, const std::string &pass);
    /**
     * @brief getAuthenticationsIdxs Get authentications Secret Indexes.
     * @return set of password indexes.
     */
    std::set<uint32_t> getAuthenticationsIdxs();
    /**
     * @brief getAuthentication Get authentication object given a password index.
     * @param idx Authentication Secret Index.
     * @return Authentication Object.
     */
    Data getAuthentication( const uint32_t & idx );


private:
    std::map<uint32_t,Data> authentications;
};

}}
#endif // REQUEST_H
