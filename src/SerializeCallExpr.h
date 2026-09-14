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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class CallExprSerializer
    {
        const ContextItems& contextItems;
        const CallExpr* callExpr;
    public:
        CallExprSerializer(const ContextItems& contextItems, const CallExpr* callExpr) : contextItems(contextItems), callExpr(callExpr) {}
        std::string Serialize() const
        {
            std::string out;
            out += SerializeExpr(contextItems, callExpr->getCallee()->IgnoreImpCasts());
            out += "(";

            bool first = true;
            for (const clang::Expr* arg : callExpr->arguments()) {
                if (first)
                    first = false;
                else
                    out += ", ";
                out += SerializeExpr(contextItems, arg);
            }
            out += ")";
            return out;
        }
    };
}