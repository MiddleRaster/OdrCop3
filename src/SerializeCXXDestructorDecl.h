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

#include "SerializeFunctionDecl.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class CXXDestructorDeclSerializer : private FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
        const ContextItems     & contextItems;
        const CXXDestructorDecl* cxxDestructorDecl;

    public:
        CXXDestructorDeclSerializer(const ContextItems& contextItems, const CXXDestructorDecl* cxxDestructorDecl)
            : FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, dyn_cast<FunctionDecl>(cxxDestructorDecl))
            , contextItems(contextItems), cxxDestructorDecl(cxxDestructorDecl)
        {}
        std::string Serialize() const
        {
            return FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize(
                [&](){ return ""; },
                [&](){ return FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>::get_FunctionName(); });
        }
    };
}