#include "json.h"


std::string Mantids29::Helpers::jsonToString(const json &value)
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::string xstrValue = Json::writeString(builder, value);

    if (!xstrValue.empty() && xstrValue[xstrValue.length()-1] == '\n')
    {
        xstrValue.erase(xstrValue.length()-1);
    }
    return xstrValue;
}

Mantids29::Helpers::JSONReader2::JSONReader2()
{
    Json::CharReaderBuilder builder;
    m_reader = builder.newCharReader();
}

Mantids29::Helpers::JSONReader2::~JSONReader2()
{
    delete m_reader;
}

bool Mantids29::Helpers::JSONReader2::parse(const std::string &document, Json::Value &root)
{
    return m_reader->parse(document.c_str(),document.c_str()+document.size(),&root,&m_errors);
}

std::string Mantids29::Helpers::JSONReader2::getFormattedErrorMessages()
{
    return m_errors;
}

std::list<std::string> Mantids29::Helpers::jsonToStringList(const json &value, const std::string &sub)
{
    std::list<std::string> r;

    if (sub.empty() && value.isArray())
    {
        for ( size_t x = 0; x< value.size(); x++)
        {
            if (value[(int)x].isString())
                r.push_back(value[(int)x].asString());
        }
    }
    else if (!sub.empty() && JSON_ISARRAY(value,sub))
    {
        for ( size_t x = 0; x< value[sub].size(); x++)
        {
            if (value[sub][(int)x].isString())
                r.push_back(value[sub][(int)x].asString());
        }
    }
    return r;
}

json Mantids29::Helpers::setToJson(const std::set<std::string> &t)
{
    json x;
    int v=0;
    for (const std::string & i : t)
        x[v++] = i;
    return x;
}

json Mantids29::Helpers::setToJson(const std::set<uint32_t> &t)
{
    json x;
    int v=0;
    for (const uint32_t & i : t)
        x[v++] = i;
    return x;
}
