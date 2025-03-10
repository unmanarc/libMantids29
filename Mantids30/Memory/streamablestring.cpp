#include "streamablestring.h"
#include <string.h>

using namespace Mantids30::Memory::Streams;

bool StreamableString::streamTo(std::shared_ptr<Memory::Streams::StreamableObject> out, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;
    if (!(cur=out->writeFullStream(m_value.c_str(),m_value.size(),wrStatUpd)).succeed || cur.finish)
    {
        if (!cur.succeed)
        {
            out->writeEOF(false);
            return false;
        }
        else
        {
            out->writeEOF(true);
            return true;
        }
    }
    out->writeEOF(true);
    return true;
}

StreamableObject::Status StreamableString::write(const void *buf, const size_t &count, StreamableObject::Status &wrStatUpd)
{
    StreamableObject::Status cur;

    std::string x( (static_cast<const char *>(buf)), count);
    m_value+=x;

    cur+=static_cast<uint64_t>(count);
    wrStatUpd+=static_cast<uint64_t>(count);
    return  cur;
}

StreamableString &StreamableString::operator=(const std::string &str) {
    setValue(str);
    return *this;
}

const std::string &StreamableString::getValue() const
{
    return m_value;
}

void StreamableString::setValue(const std::string &newValue)
{
    m_value = newValue;
}
