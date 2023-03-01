#ifndef A_INT8_H
#define A_INT8_H

#include "a_var.h"
#include <stdint.h>
#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Memory { namespace Abstract {

class INT8: public Var
{
public:
    INT8();
    INT8(const int8_t &value);
    INT8& operator=(const int8_t & value)
    {
        setValue(value);
        return *this;
    }

    int8_t getValue();
    bool setValue(const int8_t & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    int8_t value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_INT8_H
