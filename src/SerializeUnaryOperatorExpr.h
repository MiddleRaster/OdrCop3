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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class UnaryOperatorExprSerializer
    {
        const ContextItems & contextItems;
        const UnaryOperator* unaryOperator;
    public:
        UnaryOperatorExprSerializer(const ContextItems& contextItems, const UnaryOperator* unaryOperator) : contextItems(contextItems), unaryOperator(unaryOperator) {}
        std::string Serialize() const
        {
            std::string out;
            switch (unaryOperator->getOpcode())
            {
            case clang::UO_AddrOf: out += "&"; break;
            case clang::UO_Deref:  out += "*"; break;
            case clang::UO_Minus:  out += "-"; break;
            case clang::UO_Plus:   out += "+"; break;
            case clang::UO_Not:    out += "!"; break;
            case clang::UO_LNot:   out += "~"; break;
            default:                           break;
            }
            out += IndentBlock(SerializeExpr(contextItems, unaryOperator->getSubExpr()), 1);
            return out;
        }
    };
}