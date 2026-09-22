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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class EnumConstantDeclSerializer
    {
        const ContextItems    & contextItems;
        const EnumConstantDecl* enumConstantDecl;
    public:
        EnumConstantDeclSerializer(const ContextItems& contextItems, const EnumConstantDecl* enumConstantDecl) : contextItems(contextItems), enumConstantDecl(enumConstantDecl) {}
        std::string Serialize() const
        {
            std::string out;
            out += enumConstantDecl->getQualifiedNameAsString();
            if (const Expr* expr = enumConstantDecl->getInitExpr())
                out += " = " + IndentBlock(SerializeExpr(contextItems, expr), 3 + LengthOfLastLine(out));
            out += ";\n";
            return out;
        }
    };
}