#pragma once

#include <memory>
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

    AtomicExpression( std::shared_ptr<std::vector<std::string>> staticTexts );

    bool compile( std::string expr );
    bool evaluate(const json & values);

    void setStaticTexts(std::shared_ptr<std::vector<std::string>> value);

private:
    bool calcNegative(bool r);
    bool substractExpressions(const std::string &regex, const eEvalOperator & op);

    std::shared_ptr<std::vector<std::string>> m_staticTexts;
    std::string m_expr;
    AtomicExpressionSide m_left,m_right;
    eEvalOperator m_evalOperator;
    bool m_negativeExpression, m_ignoreCase;

};

}}}
