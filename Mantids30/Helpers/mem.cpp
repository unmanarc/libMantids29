#include "mem.h"
#include <cstdlib>
#include <stdexcept>
#include <string.h>

using namespace std;
using namespace Mantids30::Helpers;

unsigned char Mem::m_cmpMatrix[] =
{
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
    //65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,
    97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,
    91,92,93,94,95,96,
    97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,
    123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

Mem::BinaryDataContainer::BinaryDataContainer(const char *_data, const size_t &len)
{
    if (len)
    {
        this->data = malloc(len);
    }
    else
    {
        this->data = nullptr;
    }

    if (this->data)
    {
        this->length = len;
        memcpy(this->data, _data, len);
    }
    else
    {
        this->length = 0;
    }

    cur = 0;
}

Mem::BinaryDataContainer::BinaryDataContainer(
    const size_t &len)
{
    if (len)
    {
        data = malloc(len);
    }
    else
    {
        data = nullptr;
    }

    if (data)
    {
        this->length = len;
        memset(data, 0, len);
    }
    else
    {
        this->length = 0;
    }

    cur = 0;
}

Mem::BinaryDataContainer::~BinaryDataContainer()
{
    if (data)
    {
        free(data);
    }
    data = nullptr;
    length = 0;
    cur = 0;
}

void Mem::BinaryDataContainer::operator+=(
    const unsigned char &c)
{
    if (cur >= length) {
        throw runtime_error("Attempt to append to a full BinaryDataContainer");
    }
    unsigned char *data_c = reinterpret_cast<unsigned char *>(data);
    data_c[cur++] = c;
}

bool Mem::icharcmp(
    unsigned char c1, unsigned char c2)
{
    return m_cmpMatrix[c1] == m_cmpMatrix[c2];
}

int Mem::memcmp64(
    const void *s1, const void *s2, size_t n)
{
    constexpr size_t blockSize = 64 * KB_MULT;
    for (size_t offset = 0; offset < n; offset += blockSize) {
        size_t currentBlock = std::min(n - offset, blockSize);
        int result = memcmp(static_cast<const char *>(s1) + offset, static_cast<const char *>(s2) + offset, currentBlock);
        if (result != 0)
            return result;
    }
    return 0;
}

int Mem::memicmp2(
    const void *s1, const void *s2, const size_t &n, const bool &caseSensitive)
{
    if (caseSensitive)
    {
        return memcmp64(s1, s2, n);
    }
    else
    {
        const unsigned char* p1 = static_cast<const unsigned char*>(s1);
        const unsigned char* p2 = static_cast<const unsigned char*>(s2);
        for (size_t i = 0; i < n; ++i)
        {
            if (!icharcmp(p1[i], p2[i]))
                return -1;
        }
        return 0;
    }
}

void *Mem::memcpy64(
    void *destination, const void *source, size_t numBytes)
{
    size_t currentOffset = 0;
    while (numBytes > 0)
    {
        size_t blockSizeToCopy = std::min(numBytes, static_cast<size_t>(64 * KB_MULT));
        memcpy(static_cast<char *>(destination) + currentOffset, static_cast<const char *>(source) + currentOffset, blockSizeToCopy);
        currentOffset += blockSizeToCopy;
        numBytes -= blockSizeToCopy;
        currentOffset += blockSizeToCopy;
        numBytes -= blockSizeToCopy;
    }
    return destination;
}
void *Mem::memmove64(
    void *dest, const void *src, size_t numBytes)
{
    size_t blockSize = 64 * KB_MULT;
    if (dest > src)
    {
        for (size_t currentBlock = (numBytes > blockSize ? blockSize : numBytes); numBytes; numBytes -= currentBlock)
        {
            memmove(static_cast<char *>(dest) + numBytes - currentBlock, static_cast<const char *>(src) + numBytes - currentBlock, currentBlock);
        }
    }
    else if (dest < src)
    {
        size_t currentOffset = 0;
        for (size_t currentBlock = (numBytes > blockSize ? blockSize : numBytes); numBytes; numBytes -= currentBlock)
        {
            memmove(static_cast<char *>(dest) + currentOffset, static_cast<const char *>(src) + currentOffset, currentBlock);
            currentOffset += currentBlock;
        }
    }
    return dest;
}
