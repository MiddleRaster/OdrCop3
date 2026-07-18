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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class VarTemplatePartialSpecializationDeclSerializer
    {
        const ContextItems                        & contextItems;
        const VarTemplatePartialSpecializationDecl* varTemplatePartialSpecializationDecl;
    public:
        VarTemplatePartialSpecializationDeclSerializer(const ContextItems& contextItems, const VarTemplatePartialSpecializationDecl* varTemplatePartialSpecializationDecl) : contextItems(contextItems), varTemplatePartialSpecializationDecl(varTemplatePartialSpecializationDecl) {}
        std::string Serialize() const
        {
            std::string                 templatePrefix;
            llvm::raw_string_ostream os(templatePrefix);
            varTemplatePartialSpecializationDecl->getTemplateParameters()->print(os, contextItems.context, contextItems.printPolicy);
            os.flush();

            // change "template <" to "template<"
            std::string::size_type pos = templatePrefix.find("template <");
            if (pos != std::string::npos)
                templatePrefix.replace(pos, 10, "template<");

            return templatePrefix + IndentBlock(VarDeclSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, static_cast<const clang::VarDecl*>(varTemplatePartialSpecializationDecl)).Serialize(), 0);
        }
    };
}
