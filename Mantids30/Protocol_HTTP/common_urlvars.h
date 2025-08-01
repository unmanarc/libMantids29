#pragma once

#include <Mantids30/Memory/parser.h>
#include <Mantids30/Memory/vars.h>
#include <map>
#include <memory>

#include "common_urlvar_subparser.h"

namespace Mantids30 {
namespace Network {
namespace Protocols {
namespace HTTP {

class URLVars : public Memory::Abstract::Vars, public Memory::Streams::Parser
{
public:
    enum eHTTP_URLVarStat
    {
        URLV_STAT_WAITING_NAME,
        URLV_STAT_WAITING_CONTENT
    };

    static std::shared_ptr<URLVars> create(std::shared_ptr<StreamableObject> value = nullptr);
    ~URLVars() override = default;

    /////////////////////////////////////////////////////
    // Stream Parsing:
    bool streamTo(Memory::Streams::StreamableObject *out) override;

    /////////////////////////////////////////////////////
    // Variables Container:
    /**
     * @brief varCount Get the number of values for a variable name
     * @param varName Variable name
     * @return Count of values
     */
    uint32_t varCount(const std::string &varName) override;
    /**
     * @brief getValue Get Value of specific variable
     * @param varName Variable Name
     * @return binary container with the variable
     */
    std::shared_ptr<Memory::Streams::StreamableObject> getValue(const std::string &varName) override;
    /**
     * @brief getValues Get the values for an specific variable name
     * @param varName variable name
     * @return list of binary containers containing the data of each value of the variable
     */
    std::list<std::shared_ptr<Memory::Streams::StreamableObject>> getValues(const std::string &varName) override;
    /**
     * @brief getKeysList Get set of Variable Names
     * @return set of variables names
     */
    std::set<std::string> getKeysList() override;
    /**
     * @brief isEmpty Return if there is no vars
     * @return false for no variable defined
     */
    bool isEmpty() override;
    /**
     * @brief addVar Add Variable (Useful for Requests)
     * @param varName Var Name
     * @param data Variable Data (will be destroyed during URLVars destruction)
     */
    bool addVar(const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data) override;
    /**
     * @brief clear all vars.
     */
    void clear() override
    {
        m_vars.clear();
    }

    size_t size() override;

protected:
    void iSetMaxVarContentSize() override;
    void iSetMaxVarNameSize() override;

    bool initProtocol() override;
    void endProtocol() override;
    bool changeToNextParser() override;

private:
    URLVars(std::shared_ptr<StreamableObject> value = nullptr);

    eHTTP_URLVarStat m_currentStat = URLV_STAT_WAITING_NAME;

    std::string m_currentVarName;
    std::multimap<std::string, std::shared_ptr<Memory::Containers::B_Chunks>> m_vars;

    URLVarContent m_urlVarParser;
};
} // namespace HTTP
} // namespace Protocols
} // namespace Network
} // namespace Mantids30
