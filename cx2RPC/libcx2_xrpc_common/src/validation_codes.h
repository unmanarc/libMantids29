#ifndef VALIDATION_RETCODES_H
#define VALIDATION_RETCODES_H

namespace CX2 { namespace RPC {
enum eMethodValidationCodes
{
    VALIDATION_OK = 0x0,
    VALIDATION_METHODNOTFOUND = 0x1,
    VALIDATION_NOTAUTHORIZED = 0x2
};
}}

#endif // VALIDATION_RETCODES_H
