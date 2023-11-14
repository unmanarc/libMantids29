#pragma once

#include <string>
#include <vector>

#include <Mantids30/Helpers/json.h>

#include "atomicexpressionside.h"

namespace Mantids30 { namespace Scripts { namespace Expressions {



class AtomicExpression
{
public:
    enum eEvalOperator {
        EVAL_OPERATOR_CONTAINS, // operator for multi items..
        EVAL_OPERATOR_REGEXMATCH,
        EVAL_OPERATOR_ISEQUAL,
        EVAL_OPERATOR_STARTSWITH,
        EVAL_OPERATOR_ENDSWITH,
        EVAL_OPERATOR_ISNULL,
        EVAL_OPERATOR_UNDEFINED
    };

    AtomicExpression(std::vector<std::string> *staticTexts );

    bool compile( std::string expr );
    bool evaluate(const json & values);

    void setStaticTexts(std::vector<std::string> *value);

private:
    bool calcNegative(bool r);
    bool substractExpressions(const std::string &regex, const eEvalOperator & op);

    std::vector<std::string> *staticTexts;
    std::string expr;
    AtomicExpressionSide left,right;
    eEvalOperator evalOperator;
    bool negativeExpression, ignoreCase;

};

}}}
