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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TypeAliasDeclSerializer
    {
        const ContextItems & contextItems;
        const TypeAliasDecl* typeAliasDecl;
    public:
        TypeAliasDeclSerializer(const ContextItems& contextItems, const TypeAliasDecl* typeAliasDecl) : contextItems(contextItems), typeAliasDecl(typeAliasDecl) {}

        std::string Serialize() const
        {
            std::string fqtd;
            fqtd = "using " + typeAliasDecl->getNameAsString() + " = ";
            fqtd += TrimRightIf(IndentBlock(SerializeType(contextItems, typeAliasDecl->getUnderlyingType()), fqtd.size()), ";");
            fqtd += ";\n";
            return fqtd;
        }
    };
}