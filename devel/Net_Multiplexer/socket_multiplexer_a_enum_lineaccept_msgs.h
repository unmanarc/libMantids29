#pragma once


namespace Mantids { namespace Network { namespace Multiplexor { namespace DataStructs {

enum eLineAcceptAnswerMSG {
    INIT_LINE_ANS_THREADED         =0x00,
    INIT_LINE_ANS_ESTABLISHED      =0x01,
    INIT_LINE_ANS_THREADFAILED     =0x02,
    INIT_LINE_ANS_BADPARAMS        =0x03,
    INIT_LINE_ANS_FAILED           =0x04,
    INIT_LINE_ANS_NOCALLBACK       =0x05,
    INIT_LINE_ANS_BADSERVERSOCK    =0x06,
    INIT_LINE_ANS_BADLOCALLINE     =0x07,
    INIT_LINE_ANS_NOTAUTHORIZED    =0x08
};

}}}}

