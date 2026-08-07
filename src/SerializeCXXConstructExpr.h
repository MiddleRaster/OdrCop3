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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class CXXConstructExprSerializer
    {
        const ContextItems    & contextItems;
        const CXXConstructExpr* cxxConstructExpr;
    public:
        CXXConstructExprSerializer(const ContextItems& contextItems, const CXXConstructExpr* cxxConstructExpr) : contextItems(contextItems), cxxConstructExpr(cxxConstructExpr) {}
        std::string Serialize() const
        {
            std::string out;
            for (unsigned i=0; i<cxxConstructExpr->getNumArgs(); ++i) {
                if (i > 0)
                    out += ", ";
                out += SerializeExpr(contextItems, cxxConstructExpr->getArg(i));
            }
            return out;
        }
    };
}