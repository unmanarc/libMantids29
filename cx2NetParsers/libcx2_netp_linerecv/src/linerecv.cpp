#include "linerecv.h"

using namespace CX2::Network::HTTP;

LineRecv::LineRecv(Memory::Streams::Streamable *sobject) : Memory::Streams::Parsing::Parser(sobject,false)
{
    initialized = initProtocol();
    currentParser = (Memory::Streams::Parsing::SubParser *)(&subParser);
}

void LineRecv::setMaxLineSize(const uint32_t &maxLineSize)
{
    subParser.setMaxObjectSize(maxLineSize);
}

bool LineRecv::initProtocol()
{
    return true;
}

bool LineRecv::changeToNextParser()
{
    if (!processParsedLine( subParser.getParsedString() ))
    {
        currentParser = nullptr;
        return false;
    }
    return true;
}


