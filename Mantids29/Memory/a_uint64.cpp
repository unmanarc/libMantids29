#include "a_uint64.h"
#include <stdexcept>      // std::invalid_argument
#include <Mantids29/Threads/lock_shared.h>

using namespace Mantids29::Memory::Abstract;

UINT64::UINT64()
{
    value = 0;
    setVarType(TYPE_UINT64);
}

UINT64::UINT64(const uint64_t &value)
{
    setVarType(TYPE_UINT64);
    this->value = value;
}

uint64_t UINT64::getValue()
{
    Threads::Sync::Lock_RD lock(mutex);

    return value;
}

int64_t UINT64::getIValueTruncatedOrZero()
{
    Threads::Sync::Lock_RD lock(mutex);

    if (value<=0x7FFFFFFFFFFFFFFF)
        return value;
    else
        return 0;
}

bool UINT64::setValue(const uint64_t &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    this->value = value;
    return true;
}

std::string UINT64::toString()
{
    Threads::Sync::Lock_RD lock(mutex);

    return std::to_string(value);
}

bool UINT64::fromString(const std::string &value)
{
    Threads::Sync::Lock_RW lock(mutex);

    if (value.empty())
    {
        this->value = 0;
        return true;
    }

    this->value = strtoull( value.c_str(), nullptr, 10 );
    if (value!="0" && this->value==0) return false;

    return true;
}

Var *UINT64::protectedCopy()
{
    Threads::Sync::Lock_RD lock(mutex);

    UINT64 * var = new UINT64;
    if (var) *var = this->value;
    return var;
}
