#include "mime_partmessage.h"

using namespace Mantids30::Network::Protocols::MIME;


MIME_PartMessage::MIME_PartMessage()
{
}

bool MIME_PartMessage::stream(Memory::Streams::StreamableObject::Status &wrStat)
{
    if (!header.stream(wrStat)) return false;
    if (!content.stream(wrStat)) return false;
    return true;
}


MIME_Sub_Content *MIME_PartMessage::getContent()
{
    return &content;
}

MIME_Sub_Header * MIME_PartMessage::getHeader()
{
    return &header;
}

