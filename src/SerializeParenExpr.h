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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ParenExprSerializer
    {
        const ContextItems& contextItems;
        const ParenExpr   * parenExpr;
    public:
        ParenExprSerializer(const ContextItems& contextItems, const ParenExpr* parenExpr) : contextItems(contextItems), parenExpr(parenExpr) {}
        std::string Serialize() const
        {
             return "(" + IndentBlock(SerializeExpr(contextItems, parenExpr->getSubExpr()), 1) + ") ";
        }
    };
}