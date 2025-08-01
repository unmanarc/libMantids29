#pragma once

#include <Mantids30/Memory/subparser.h>

namespace Mantids30 { namespace Network { namespace Protocols { namespace Line2Line {

class LineRecv_SubParser : public Memory::Streams::SubParser
{
public:
    LineRecv_SubParser();
    ~LineRecv_SubParser() override = default;

    void setMaxObjectSize(const uint32_t &size);

    std::string getParsedString() const;

protected:
    std::string m_parsedString;
    Memory::Streams::SubParser::ParseStatus parse() override;

};
}}}}

