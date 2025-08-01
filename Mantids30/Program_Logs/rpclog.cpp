#include "rpclog.h"
#include "loglevels.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#include <shlobj.h>
#else
#include <arpa/inet.h>
#include <pwd.h>
#include <syslog.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include <Mantids30/Helpers/encoders.h>

using namespace Mantids30::Program::Logs;

RPCLog::RPCLog(unsigned int _logMode) : LogBase(_logMode)
{
}

void RPCLog::log(eLogLevels logSeverity, const std::string &ip, const std::string &sessionId, const std::string &user, const std::string &domain, const std::string &module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);
    logVA(logSeverity,ip,sessionId,user,domain,module,outSize,fmtLog,args);
    va_end(args);
}


void RPCLog::logVA(eLogLevels logSeverity, const std::string &ip, const std::string &sessionId, const std::string &user, const std::string &domain, const std::string &module, const uint32_t &outSize, const char *fmtLog, va_list args)
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    char * buffer = new char[outSize];
    if (!buffer) 
        return;

    // take arguments...
    vsnprintf(buffer, outSize, fmtLog, args);

    if (logSeverity == LEVEL_INFO)
        printStandardLog(logSeverity,stdout,ip,sessionId,user,domain,module,buffer,LOG_COLOR_BOLD,"INFO");
    else if (logSeverity == LEVEL_WARN)
        printStandardLog(logSeverity,stdout,ip,sessionId,user,domain,module,buffer,LOG_COLOR_BLUE,"WARN");
    else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && m_debug)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_GREEN,"DEBUG");
    else if (logSeverity == LEVEL_CRITICAL)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_RED,"CRIT");
    else if (logSeverity == LEVEL_SECURITY_ALERT)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_ORANGE,"SECU");
    else if (logSeverity == LEVEL_ERR)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_PURPLE,"ERR");

    fflush(stderr);
    fflush(stdout);

    delete [] buffer;
}

void RPCLog::printStandardLog(eLogLevels logSeverity,FILE *fp, std::string ip, std::string sessionId, std::string user, std::string domain, std::string module, const char *buffer, eLogColors color, const char *logLevelText)
{
    if (true)
    {
        std::unique_lock<std::mutex> lock(m_modulesOutputExclusionMutex);
        if (m_modulesOutputExclusion.find(module)!=m_modulesOutputExclusion.end()) 
        return;
    }

    user = Helpers::Encoders::toURL(user,Helpers::Encoders::QUOTEPRINT_ENCODING);
    domain = Helpers::Encoders::toURL(domain,Helpers::Encoders::QUOTEPRINT_ENCODING);
    sessionId = Helpers::Encoders::toURL(truncateSessionId(sessionId),Helpers::Encoders::QUOTEPRINT_ENCODING);

    if (!enableAttributeNameLogging && enableEmptyFieldLogging)
    {
        if (ip.empty()) ip="-";
        if (sessionId.empty()) sessionId="-";
        if (user.empty()) user="-";
        if (domain.empty()) domain="-";
        if (module.empty()) module="-";
    }

    std::string logLine;

    if (enableAttributeNameLogging)
    {
        if ((ip.empty() && enableEmptyFieldLogging) || !ip.empty())
            logLine += "IPADDR=" + getAlignedValue("\"" + ip + "\"",INET_ADDRSTRLEN) + fieldSeparator;
        if ((sessionId.empty() && enableEmptyFieldLogging) || !sessionId.empty())
            logLine += "SESSID=" + getAlignedValue("\"" + sessionId + "\"",15) + fieldSeparator;
        if ((user.empty() && enableEmptyFieldLogging) || !user.empty())
            logLine += "USER=" + getAlignedValue("\"" + user + "\"",userFieldMinWidth) +  fieldSeparator;
        if (((domain.empty() && enableEmptyFieldLogging) || !domain.empty()) && enableDomainLogging)
            logLine += "DOMAIN=" + getAlignedValue("\"" + domain + "\"",domainFieldMinWidth) +  fieldSeparator;
        if (((module.empty() && enableEmptyFieldLogging) || !module.empty()) && enableModuleLogging)
            logLine += "MODULE=" + getAlignedValue("\"" + module + "\"",moduleFieldMinWidth)+ "" + fieldSeparator;
        if ((!buffer[0] && enableEmptyFieldLogging) || buffer[0])
            logLine += "LOGDATA=\"" + Helpers::Encoders::toURL(std::string(buffer),Helpers::Encoders::QUOTEPRINT_ENCODING) + "\"";
    }
    else
    {
        if ((ip.empty() && enableEmptyFieldLogging) || !ip.empty())
            logLine +=  getAlignedValue("\"" +ip+ "\"",INET_ADDRSTRLEN) + fieldSeparator ;

        if ((sessionId.empty() && enableEmptyFieldLogging) || !sessionId.empty())
            logLine += getAlignedValue("\"" +sessionId+ "\"",15) + fieldSeparator;

        if ((user.empty() && enableEmptyFieldLogging) || !user.empty())
            logLine += getAlignedValue("\"" +user+ "\"",userFieldMinWidth) +  fieldSeparator;

        if (((domain.empty() && enableEmptyFieldLogging) || !domain.empty()) && enableDomainLogging)
            logLine += getAlignedValue("\"" +domain+ "\"",domainFieldMinWidth)+  fieldSeparator;

        if (((module.empty() && enableEmptyFieldLogging) || !module.empty()) && enableModuleLogging)
            logLine +=getAlignedValue("\"" +module+ "\"",moduleFieldMinWidth) + fieldSeparator;

        if ((!buffer[0] && enableEmptyFieldLogging) || buffer[0])
            logLine += "\"" + Helpers::Encoders::toURL(std::string(buffer),Helpers::Encoders::QUOTEPRINT_ENCODING) + "\"";
    }


    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"%s", logLine.c_str());
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"%s", logLine.c_str());
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "%s",logLine.c_str());
        else if (logSeverity == LEVEL_SECURITY_ALERT)
            syslog( LOG_WARNING, "%s",logLine.c_str());
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "%s",logLine.c_str());
#endif
    }

    if (isUsingStandardLog())
    {
        fprintf(fp,"R/");
        if (enableDateLogging)
        {
            printDate(fp);
        }

        if (enableColorLogging)
        {
            if (enableAttributeNameLogging) fprintf(fp, "LEVEL=");
            switch (color)
            {
            case LOG_COLOR_NORMAL:
                fprintf(fp,"%s", getAlignedValue(logLevelText,6).c_str()); break;
            case LOG_COLOR_BOLD:
                printColorBold(fp,getAlignedValue(logLevelText,6).c_str()); break;
            case LOG_COLOR_RED:
                printColorRed(fp,getAlignedValue(logLevelText,6).c_str()); break;
            case LOG_COLOR_GREEN:
                printColorGreen(fp,getAlignedValue(logLevelText,6).c_str()); break;
            case LOG_COLOR_BLUE:
                printColorBlue(fp,getAlignedValue(logLevelText,6).c_str()); break;
            case LOG_COLOR_PURPLE:
                printColorPurple(fp,getAlignedValue(logLevelText,6).c_str()); break;
            case LOG_COLOR_ORANGE:
                printColorOrange(fp,getAlignedValue(logLevelText,6).c_str()); break;
            }
            fprintf(fp, "%s", fieldSeparator.c_str());
           // fflush(fp);

        }
        else
        {
            fprintf(fp, "%s", getAlignedValue(logLevelText,6).c_str());
            fprintf(fp, "%s", fieldSeparator.c_str());
          //  fflush(fp);

        }

        fprintf(fp, "%s\n",  logLine.c_str());
        //fflush(fp);

        fflush(stderr);
        fflush(stdout);

    }

}
std::string RPCLog::truncateSessionId(std::string sSessionId)
{
    if (sSessionId.size()>12) sSessionId.erase(12, std::string::npos);
    return sSessionId;
}


