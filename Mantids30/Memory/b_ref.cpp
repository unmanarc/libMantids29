#include "b_ref.h"

using namespace Mantids30::Memory::Containers;


B_Ref::B_Ref(B_Base *bc, const uint64_t &offset, const uint64_t &maxBytes)
{
    m_storeMethod = BC_METHOD_BCREF;
    referencedBC = nullptr;

    B_Ref::clear2();

    if (bc) reference(bc,offset,maxBytes);
}

B_Ref::~B_Ref()
{
    B_Ref::clear2();
}

bool B_Ref::reference(B_Base *bc, const uint64_t &offset, const uint64_t &maxBytes)
{
    if (offset>bc->size()) return false;

    m_readOnly = true; // :p

    referencedBC = bc;
    referencedOffset = offset;
    referencedMaxBytes = maxBytes;

    return true;
}

uint64_t B_Ref::size() const
{
    if (!referencedBC) return 0;
    uint64_t availableBytes = referencedBC->size()-referencedOffset; // Available bytes.

    if (referencedMaxBytes==std::numeric_limits<uint64_t>::max() || referencedMaxBytes>availableBytes) return availableBytes;
    else return referencedMaxBytes;
}

std::pair<bool, uint64_t> B_Ref::findChar(const int &c, const uint64_t &offset, uint64_t searchSpace, bool caseSensitive)
{
    if (!referencedBC) return std::make_pair(false,std::numeric_limits<uint64_t>::max());
    if (caseSensitive && !(c>='A' && c<='Z') && !(c>='a' && c<='z') )
        caseSensitive = false;
    return referencedBC->findChar(c, referencedOffset+offset,searchSpace, caseSensitive );
}

std::pair<bool, uint64_t> B_Ref::truncate2(const uint64_t &bytes)
{
    referencedMaxBytes = bytes;
    return std::make_pair(true,bytes);
}

std::pair<bool,uint64_t> B_Ref::append2(const void *buf, const uint64_t &len, bool prependMode)
{
    if (!referencedBC) return std::make_pair(false,(uint64_t)0); // CANT APPEND TO NOTHING.

    if (prependMode)
        return referencedBC->prepend(buf,len);
    else
        return referencedBC->append(buf,len);
}

std::pair<bool,uint64_t> B_Ref::displace2(const uint64_t &roBytesToDisplace)
{
    uint64_t bytesToDisplace=roBytesToDisplace;
    if (!referencedBC) return std::make_pair(false,(uint64_t)0);
    if (bytesToDisplace>size()) bytesToDisplace = size();
    if (referencedMaxBytes!=std::numeric_limits<uint64_t>::max())
        referencedMaxBytes-=bytesToDisplace;
    referencedOffset+=bytesToDisplace;
    return std::make_pair(true,bytesToDisplace);
}

bool B_Ref::clear2()
{
    if (referencedBC)
    {
      //  delete referencedBC;
        referencedBC = nullptr;
    }
    return true;
}

std::pair<bool,uint64_t> B_Ref::copyToStream2(std::ostream &out, const uint64_t &bytes, const uint64_t &offset)
{
    if (!referencedBC) return std::make_pair(false,(uint64_t)0);

    // CAN'T COPY BEYOND OFFSET.
    if (offset>size()) return std::make_pair(false,(uint64_t)0);

    uint64_t maxBytesToCopy = size()-offset;
    uint64_t bytesToCopy = maxBytesToCopy>bytes?bytes:maxBytesToCopy;

    return referencedBC->copyToStream(out,bytesToCopy,referencedOffset+offset);
}

std::pair<bool,uint64_t> B_Ref::copyTo2(StreamableObject &bc, Streams::StreamableObject::Status & wrStatUpd, const uint64_t &bytes, const uint64_t &offset)
{
    if (!referencedBC)
    {
        wrStatUpd.succeed=false;
        return std::make_pair(false,(uint64_t)0);
    }

    // CAN'T COPY BEYOND OFFSET.
    if (offset>size())
    {
        wrStatUpd.succeed=false;
        return std::make_pair(false,(uint64_t)0);
    }

    uint64_t maxBytesToCopy = size()-offset;
    uint64_t bytesToCopy = maxBytesToCopy>bytes?bytes:maxBytesToCopy;

    return referencedBC->appendTo(bc,wrStatUpd,bytesToCopy,referencedOffset+offset);
}

std::pair<bool,uint64_t> B_Ref::copyOut2(void *buf, const uint64_t &bytes, const uint64_t &offset)
{
    if (!referencedBC) return std::make_pair(false,(uint64_t)0);

    // CAN'T COPY BEYOND OFFSET.
    if (offset>size()) return std::make_pair(false,(uint64_t)0);

    uint64_t maxBytesToCopy = size()-offset;
    uint64_t bytesToCopy = maxBytesToCopy>bytes?bytes:maxBytesToCopy;

    return referencedBC->copyOut(buf,bytesToCopy,referencedOffset+offset);
}

bool B_Ref::compare2(const void *buf, const uint64_t &len, bool caseSensitive, const uint64_t &offset)
{
    if (!referencedBC) return false;

    // CAN'T COPY BEYOND OFFSET.
    if (offset>size()) return false;

    uint64_t maxBytesToCompare = size()-offset;
    uint64_t bytesToCompare = maxBytesToCompare>len?len:maxBytesToCompare;

    return referencedBC->compare(buf,bytesToCompare,caseSensitive,referencedOffset+offset);
}
