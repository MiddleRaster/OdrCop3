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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TemplateTypeParmTypeSerializer
    {
        const ContextItems        & contextItems;
        const TemplateTypeParmType* templateTypeParmType;
        QualType qt;
    public:
        TemplateTypeParmTypeSerializer(const ContextItems& contextItems, QualType qt, const TemplateTypeParmType* templateTypeParmType) : contextItems(contextItems), qt(qt), templateTypeParmType(templateTypeParmType) {}

        std::string Serialize() const
        {
            return templateTypeParmType->getDecl()->getNameAsString() + (contextItems.aux.size() > 0 ? " " + contextItems.aux : "");
        }
    };
}