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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class VarTemplateSpecializationDeclSerializer
    {
        const ContextItems                 & contextItems;
        const VarTemplateSpecializationDecl* varTemplateSpecializationDecl;
    public:
        VarTemplateSpecializationDeclSerializer(const ContextItems& contextItems, const VarTemplateSpecializationDecl* varTemplateSpecializationDecl) : contextItems(contextItems), varTemplateSpecializationDecl(varTemplateSpecializationDecl) {}
        std::string Serialize() const
        {
            return "template<> " + IndentBlock(VarDeclSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, static_cast<const clang::VarDecl*>(varTemplateSpecializationDecl)).Serialize(), 0) + "\n";
        }
    };
}
