#pragma once

#include <clang\AST\AST.h>
#include <clang\AST\RecursiveASTVisitor.h>
#include <clang\Frontend\FrontendActions.h>
#include <clang\Frontend\CompilerInstance.h>
#include <clang\Tooling\Tooling.h>
#include <clang\Tooling\CompilationDatabase.h>
#include <clang\AST\Mangle.h>
#include <clang\AST\Decl.h>
#include <clang\AST\GlobalDecl.h>
#include <clang\AST\RecordLayout.h>
#include <llvm\Support\raw_ostream.h>

#include "SerializationUtils.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ConditionalOperatorSerializer
    {
        const ContextItems       & contextItems;
        const ConditionalOperator* conditionalOperator;
    public:
        ConditionalOperatorSerializer(const ContextItems& contextItems, const ConditionalOperator* conditionalOperator) : contextItems(contextItems), conditionalOperator(conditionalOperator) {}
        std::string Serialize() const
        {   // the ? : operator
            std::string out;
            out += IndentBlock(SerializeExpr(contextItems, conditionalOperator->getCond     ()), LengthOfLastLine(out)) + " ? ";
            out += IndentBlock(SerializeExpr(contextItems, conditionalOperator->getTrueExpr ()), LengthOfLastLine(out)) + " : ";
            out += IndentBlock(SerializeExpr(contextItems, conditionalOperator->getFalseExpr()), LengthOfLastLine(out))        ;
            return out;
        }
    };
}