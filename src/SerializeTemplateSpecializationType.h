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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TemplateSpecializationTypeSerializer
    {
        const ContextItems              & contextItems;
        const TemplateSpecializationType* templateSpecializationType;
        QualType qt;
    public:
        TemplateSpecializationTypeSerializer(const ContextItems& contextItems, QualType qt, const TemplateSpecializationType* templateSpecializationType) : contextItems(contextItems), qt(qt), templateSpecializationType(templateSpecializationType) {}

        std::string Serialize() const
        {
            return IndentBlock(SerializeDecl(contextItems, templateSpecializationType->getTemplateName().getAsTemplateDecl()), 0);
        }
    };
}