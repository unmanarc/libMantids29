#pragma once

#include "atomicexpression.h"
#include <Mantids30/Helpers/json.h>

namespace Mantids30 { namespace Scripts { namespace Expressions {

class JSONEval
{
public:

    enum eEvalMode {
        EVAL_MODE_OR,
        EVAL_MODE_AND,
        EVAL_MODE_UNDEFINED
    };

    JSONEval();
    JSONEval(const std::string & expr);
    JSONEval(const std::string & expr, std::vector<std::string> * staticTexts, bool negativeExpression);
    ~JSONEval();

    bool compile( std::string expr );
    bool evaluate( const json & values );

    std::string getLastCompilerError() const;

    bool isCompiled() const;

private:

    bool calcNegative(bool r);

    /**
     * @brief detectSubExpr Detect and replace sub expression
     * @param expr expression string (without quotes)
     * @return -1 if failed, 0 if succeed, 1 if no more expressions
     */
    size_t detectSubExpr(std::string & expr, size_t start);

    std::string m_expression, m_lastError;
    std::vector<std::string> * m_staticTexts;
    std::vector<JSONEval *> m_subExpressions;
    std::vector<std::pair<AtomicExpression *,size_t>> m_atomExpressions;

    bool m_staticTextsOwner;
    bool m_negativeExpression;
    bool m_isCompiled;
    eEvalMode m_evaluationMode;
};

}}}

