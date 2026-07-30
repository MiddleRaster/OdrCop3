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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class DeclRefExprSerializer
    {
        const ContextItems& contextItems;
        const DeclRefExpr * declRefExpr;
    public:
        DeclRefExprSerializer(const ContextItems& contextItems, const DeclRefExpr* declRefExpr) : contextItems(contextItems), declRefExpr(declRefExpr) {}
        std::string Serialize() const
        {
            const clang::ValueDecl* valueDecl = declRefExpr->getDecl();
            if (const auto* recordDecl = dyn_cast<CXXRecordDecl>(valueDecl->getDeclContext()))
                return "(" + TrimRightIf(IndentBlock(SerializeDecl(contextItems, recordDecl), 1), ";") + ")::" + valueDecl->getNameAsString();

            return declRefExpr->getDecl()->getQualifiedNameAsString(); // fallback, in case it's not a ValueDecl* after all, so we don't know how to serialize it manually
        }
    };
}