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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class InitListExprSerializer
    {
        const ContextItems& contextItems;
        const InitListExpr* initListExpr;
    public:
        InitListExprSerializer(const ContextItems& contextItems, const InitListExpr* initListExpr) : contextItems(contextItems), initListExpr(initListExpr) {}
        std::string Serialize() const
        {
            std::string str;
            llvm::raw_string_ostream os(str);
            initListExpr->printPretty(os, nullptr, contextItems.printPolicy);
            return str;
        }
    };
}