#include "mime_sub_header.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <Mantids30/Helpers/random.h>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::MIME;
using namespace Mantids30;
using namespace std;

MIME_Sub_Header::MIME_Sub_Header()
{
    setParseMode(Memory::Streams::SubParser::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setMaxOptionSize(2*KB_MULT); // 2K per option
    maxOptions = 32; // 32 Max options
    lastOpt = nullptr;
    subParserName = "MIME_Sub_Header";
}
MIME_Sub_Header::~MIME_Sub_Header()
{
    for ( auto & i : headers )
        delete i.second;
}

bool MIME_Sub_Header::stream(Memory::Streams::StreamableObject::Status & wrStat)
{
    Memory::Streams::StreamableObject::Status cur;

    // Write out the header option values...
    for (auto & i : headers)
    {
        std::string x = i.second->getString() + std::string("\r\n");
        if (!(cur+=upStream->writeString( x, wrStat )).succeed) return false;
    }
    if (!(cur+=upStream->writeString("\r\n", wrStat)).succeed) return false;
    return true;
}

bool MIME_Sub_Header::exist(const std::string &optionName) const
{
    return getOptionByName(optionName)!=nullptr?true:false;
}

void MIME_Sub_Header::remove(const std::string &optionName)
{
    auto range = headers.equal_range(boost::to_upper_copy(optionName));

    for (auto it = range.first; it != range.second; )
    {
        delete it->second;
        it = headers.erase(it);
    }
}

void MIME_Sub_Header::replace(const std::string &optionName, const std::string &optionValue)
{
    remove(optionName);
    add(optionName,optionValue);
}

// TODO: sanitize inputs when client...
bool MIME_Sub_Header::add(const std::string &optionName, const std::string &optionValue, int state)
{
    MIME_HeaderOption * optP;
    if (state == 0)
    {
        optP = new MIME_HeaderOption;
        if (headers.size()==maxOptions)
        {
            delete optP;
            return false; // Can't exceed.
        }

        optP->setOrigName(optionName);
        parseSubValues(optP,optionValue);

        if (!addHeaderOption(optP))
        {
            delete optP;
            return false;
        }
        lastOpt = optP;
    }
    else if (state == 1 && lastOpt)
    {
        optP = lastOpt;
        parseSubValues(optP,optionValue);
    }
    return true;
}

size_t MIME_Sub_Header::getOptionsSize()
{
    return headers.size();
}

bool MIME_Sub_Header::addHeaderOption(MIME_HeaderOption *opt)
{
    if (headers.size()==maxOptions) return false; // Can't exceed.
    headers.insert(std::pair<std::string,MIME_HeaderOption *>(opt->getUpperName(),opt));
    return true;
}

std::list<MIME_HeaderOption *> MIME_Sub_Header::getOptionsByName(const std::string &varName) const
{
    std::list<MIME_HeaderOption *> values;
    auto range = headers.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) values.push_back(i->second);
    return values;
}

MIME_HeaderOption *MIME_Sub_Header::getOptionByName(const std::string &varName) const
{
    if (headers.find(boost::to_upper_copy(varName)) == headers.end()) return nullptr;
    return headers.find(boost::to_upper_copy(varName))->second;
}

std::string MIME_Sub_Header::getOptionRawStringByName(const std::string &varName) const
{
    MIME_HeaderOption * opt  = getOptionByName(varName);
    return opt?opt->getOrigValue():"";
}

std::string MIME_Sub_Header::getOptionValueStringByName(const std::string &varName) const
{
    MIME_HeaderOption * opt  = getOptionByName(varName);
    return opt?opt->getValue():"";
}

uint64_t MIME_Sub_Header::getOptionAsUINT64(const std::string &varName, uint16_t base, bool *optExist) const
{
    uint64_t r=0;
    bool lOptExist;
    if (!optExist) optExist = &lOptExist;
    MIME_HeaderOption * opt  = getOptionByName(varName);
    *optExist = opt!=nullptr?true:false;
    if (*optExist) r = strtoull(opt->getValue().c_str(),nullptr,base);
    return r;
}

void MIME_Sub_Header::setMaxOptionSize(const size_t &value)
{
    setParseDataTargetSize(value);
}

void MIME_Sub_Header::setMaxOptions(const size_t &value)
{
    maxOptions = value;
}

Memory::Streams::SubParser::ParseStatus MIME_Sub_Header::parse()
{   
    if (!getParsedBuffer()->size())
    {
#ifdef DEBUG
        printf("Parsing MIME header lines (END).\n");fflush(stdout);
#endif
        return Memory::Streams::SubParser::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }
#ifdef DEBUG
    printf("Parsing MIME header (line).\n");fflush(stdout);
#endif
    parseOptionValue(getParsedBuffer()->toString());
    return Memory::Streams::SubParser::PARSE_STAT_GET_MORE_DATA;
}

void MIME_Sub_Header::parseSubValues(MIME_HeaderOption * opt, const std::string &strName)
{
    // hello weo; doaie; fa = "hello world;" hehe; asd=399; aik=""
    std::vector<std::string> vStaticTexts;
    std::string sVarValues = strName;
    // TODO: find a way to do this easier...
    std::string secureReplace = Mantids30::Helpers::Random::createRandomString(16);
    opt->setOrigValue(sVarValues);

    boost::trim(sVarValues);

    boost::match_flag_type flags = boost::match_default;

    // PRECOMPILE _STATIC_TEXT
    boost::regex exStaticText("\"(?<STATIC_TEXT>[^\"]*)\"");
    boost::match_results<string::const_iterator> whatStaticText;
    for (string::const_iterator start = sVarValues.begin(), end =  sVarValues.end();
         boost::regex_search(start, end, whatStaticText, exStaticText, flags);
         start = sVarValues.begin(), end =  sVarValues.end())
    {
        uint64_t pos = vStaticTexts.size();
        char _staticmsg[128];

#ifdef _WIN32
        snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%llu",secureReplace.c_str(),pos);
#else
        snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%lu",secureReplace.c_str(),pos);
#endif

        vStaticTexts.push_back(string(whatStaticText[1].first, whatStaticText[1].second));
        boost::replace_all(sVarValues,"\"" + string(whatStaticText[1].first, whatStaticText[1].second) + "\"" ,_staticmsg);
    }

    vector<string> vValues;
    split(vValues,sVarValues,is_any_of(";"),token_compress_on);

    bool first = true;

    for (const std::string & value : vValues)
    {
        if (boost::contains(value,"="))
        {
            std::string sVarName = value.substr(0,value.find("="));
            std::string sVarValue = value.substr(value.find("=")+1);

            boost::trim(sVarName);
            boost::trim(sVarValue);

            // replace back...
            for (uint64_t i=0; i<vStaticTexts.size();i++)
            {
                char _staticmsg[128];
#ifdef _WIN32
                snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%llu",secureReplace.c_str(),i);
#else
                snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%lu",secureReplace.c_str(),i);
#endif
                boost::replace_all(sVarName,_staticmsg, vStaticTexts[i]);
                boost::replace_all(sVarValue,_staticmsg, vStaticTexts[i]);
            }

            opt->addSubVar(sVarName,sVarValue);
        }
        else
        {
            std::string sv = boost::trim_copy(value);

            // replace back...
            for (uint64_t i=0; i<vStaticTexts.size();i++)
            {
                char _staticmsg[128];
#ifdef _WIN32
                snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%llu",secureReplace.c_str(),i);
#else
                snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%lu",secureReplace.c_str(),i);
#endif
                boost::replace_all(sv,_staticmsg, vStaticTexts[i]);
            }

            opt->addSubVar(sv,"");
        }

        if (first)
        {
            std::string sv = boost::trim_copy(value);

            // replace back...
            for (uint64_t i=0; i<vStaticTexts.size();i++)
            {
                char _staticmsg[128];
#ifdef _WIN32
                snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%llu",secureReplace.c_str(),i);
#else
                snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%s_%lu",secureReplace.c_str(),i);
#endif
                boost::replace_all(sv,_staticmsg, vStaticTexts[i]);
            }

            opt->setValue( sv );
            first=false;
        }

    }
}

void MIME_Sub_Header::parseOptionValue(std::string optionValue)
{
    if (*(optionValue.c_str())==' ' || *(optionValue.c_str())=='\t')
    {
        // Continue on the last option...
        add("",optionValue,1);
    }
    else
    {
        size_t found=optionValue.find(": ");

        if (found!=std::string::npos)
        {
            // We have parameters..
            std::string optionValue2 = optionValue.c_str()+found+2;
            optionValue.resize(found);
            add(optionValue, optionValue2,0);
        }
        else
        {
            // bad option!
        }
    }

}

size_t MIME_Sub_Header::getMaxSubOptionSize() const
{
    return maxSubOptionSize;
}

void MIME_Sub_Header::setMaxSubOptionSize(const size_t &value)
{
    maxSubOptionSize = value;
}

size_t MIME_Sub_Header::getMaxSubOptionCount() const
{
    return maxSubOptionCount;
}

void MIME_Sub_Header::setMaxSubOptionCount(const size_t &value)
{
    maxSubOptionCount = value;
}

std::string MIME_HeaderOption::getString()
{
    std::string r;

    r = origName + ": " + origValue;

    return r;
}


bool MIME_HeaderOption::isPermited7bitCharset(const std::string &varX)
{
    // No strange chars on our vars...
    for (size_t i=0;i<varX.size();i++)
    {
        if (varX[i]<32 || varX[i]>126) return false;
    }
    return true;
}


void MIME_HeaderOption::addSubVar(const std::string &varName, const std::string &varValue)
{
    if (varName.empty() && varValue.empty()) return;
    if (subVar.size()>=maxSubOptionsCount) return;
    if (varName.size()+varValue.size()+curHeaderOptSize>maxHeaderOptSize) return;
    if (!isPermited7bitCharset(varName)) return;
    if (!isPermited7bitCharset(varValue)) return;
    curHeaderOptSize += varName.size()+varValue.size();
    // TODO: the subVar may be URL Encoded... (decode)
    subVar.insert(std::make_pair(varName,varValue));
}

std::string MIME_HeaderOption::getUpperName() const
{
    return boost::to_upper_copy(origName);
}

std::string MIME_HeaderOption::getOrigName() const
{
    return origName;
}

void MIME_HeaderOption::setOrigName(const std::string &value)
{
    origName = value;
}

std::string MIME_HeaderOption::getValue() const
{
    return value;
}

void MIME_HeaderOption::setValue(const std::string &value)
{
    this->value = value;
    boost::trim(this->value);
}

std::string MIME_HeaderOption::getOrigValue() const
{
    return origValue;
}

void MIME_HeaderOption::setOrigValue(const std::string &value)
{
    origValue = value;
}

uint64_t MIME_HeaderOption::getMaxSubOptions() const
{
    return maxSubOptionsCount;
}

void MIME_HeaderOption::setMaxSubOptions(const uint64_t &value)
{
    maxSubOptionsCount = value;
}
