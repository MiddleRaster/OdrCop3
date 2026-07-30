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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ConstantExprSerializer
    {
        const ContextItems& contextItems;
        const ConstantExpr* constantExpr;
    public:
        ConstantExprSerializer(const ContextItems& contextItems, const ConstantExpr* constantExpr) : contextItems(contextItems), constantExpr(constantExpr) {}
        std::string Serialize() const
        {
            return SerializeExpr(contextItems, constantExpr->getSubExpr());
        }
    };
}