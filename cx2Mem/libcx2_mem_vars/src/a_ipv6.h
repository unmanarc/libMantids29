#ifndef A_IPV6_H
#define A_IPV6_H

#include "a_var.h"
#ifdef _WIN32
#include <winsock2.h>
#include <in6addr.h>
#else
#include <netinet/in.h>
#endif
#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Memory { namespace Abstract {

class IPV6: public Var
{
public:
    IPV6();
    ~IPV6() override;
    IPV6& operator=(const in6_addr & value)
    {
        setValue(value);
        return *this;
    }

    in6_addr getValue();
    bool setValue(const in6_addr & value);

    void * getDirectMemory() override { return &value; }

    std::string toString() override;
    bool fromString(const std::string & value) override;
protected:
    Var * protectedCopy() override;

private:
    in6_addr value;
    Threads::Sync::Mutex_Shared mutex;

};

}}}

#endif // A_IPV6_H
