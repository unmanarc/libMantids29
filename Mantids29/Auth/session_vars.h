#pragma once

#include <Mantids29/Threads/mutex_shared.h>
#include <string>
#include <map>

namespace Mantids29 {
namespace Authentication {

/**
 * @brief The Session_Vars class manages session variables for an authenticated session in the application.
 *
 * This class manages session variables that persist throughout the duration of an authenticated session.
 */
class Session_Vars
{
public:
    Session_Vars();
    /**
     * @brief Sets the value of a session variable.
     * @param varName The name of the session variable to set.
     * @param varValue The value to set for the session variable.
     */
    void setSessionVariable(const std::string& varName, const std::string& varValue);

    /**
     * @brief Returns the value of a session variable.
     * @param varName The name of the session variable to get the value for.
     * @return The value of the session variable.
     */
    std::string getSessionVariableValue(const std::string & varName);

    /**
     * @brief Checks whether a session variable exists.
     * @param varName The name of the session variable to check for.
     * @return true if the session variable exists, false otherwise.
     */
    bool doesSessionVariableExist(const std::string& varName);

    /**
     * @brief Clears the value of a session variable.
     * @param varName The name of the session variable to clear.
     */
    void clearSessionVariable(const std::string& varName);


private:
    // This variable will persist throughout the authenticated session
    std::map<std::string, std::string> m_sessionVariables;

    // Mutex for thread safety
    Threads::Sync::Mutex_Shared m_sessionVarsMutex;
};

}} // namespace Mantids29::Authentication


