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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TypeAliasTemplateDeclSerializer
    {
        const ContextItems         & contextItems;
        const TypeAliasTemplateDecl* typeAliasTemplateDecl;
    public:
        TypeAliasTemplateDeclSerializer(const ContextItems& contextItems, const TypeAliasTemplateDecl* typeAliasTemplateDecl) : contextItems(contextItems), typeAliasTemplateDecl(typeAliasTemplateDecl) {}
        std::string Serialize() const
        {
            std::string fqtd;
            fqtd  = ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeExpr>(contextItems, typeAliasTemplateDecl->getTemplateParameters());
            fqtd += "using " + typeAliasTemplateDecl->getNameAsString() + " = ";
            fqtd += IndentBlock(SerializeType(contextItems, typeAliasTemplateDecl->getTemplatedDecl()->getUnderlyingType()), fqtd.size());
            fqtd  = TrimRightIf(fqtd, ";");
            fqtd += ";\n";
            return fqtd;
        }
    };
}