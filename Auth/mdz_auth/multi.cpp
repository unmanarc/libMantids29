#include "multi.h"
//#include "retcodes.h"

using namespace Mantids::Authentication;
using namespace Mantids;

Multi::Multi()
{
    clear();
}


std::set<uint32_t> Multi::getAuthenticationsIdxs()
{
    std::set<uint32_t> r;
    for (const auto & i : authentications)
        r.insert(i.first);
    return r;
}

Data Multi::getAuthentication(const uint32_t &idx)
{
    if (authentications.find(idx) != authentications.end())
        return authentications[idx];

    Data r;
    return r;
}
/*

void Multi::print()
{
    for (const auto & i : authentications)
    {
        Memory::Streams::StreamableJSON s;
        s.setValue(i.second.toJSON());
        std::cout << ">>>> With auth: " << s.getString() << std::endl << std::flush;
    }
}*/

bool Multi::setAuthentications(const std::string &sAuthentications)
{
    if (sAuthentications.empty()) return true;

    json jAuthentications;
    Mantids::Helpers::JSONReader2 reader;
    if (!reader.parse(sAuthentications, jAuthentications)) return false;

    return setJSON(jAuthentications);
}

bool Multi::setJSON(const json &jAuthentications)
{
    if (!jAuthentications.isObject()) return false;

    if (jAuthentications.isObject())
    {
        for (const auto &idx : jAuthentications.getMemberNames())
        {
            if ( jAuthentications[idx].isMember("pass") )
            {
                addAuthentication(strtoul(idx.c_str(),nullptr,10), JSON_ASSTRING(jAuthentications[idx],"pass",""));
            }
        }
    }

    return true;
}


void Multi::clear()
{
    authentications.clear();
}

void Multi::addAuthentication(const Data &auth)
{
    authentications[auth.getPassIndex()] = auth;
}

void Multi::addAuthentication(uint32_t passIndex,const std::string &pass)
{
    authentications[passIndex].setPassIndex(passIndex);
    authentications[passIndex].setPassword(pass);
}
