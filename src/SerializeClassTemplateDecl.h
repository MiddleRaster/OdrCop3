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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ClassTemplateDeclSerializer : private TemplateDeclBaseSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
        const ClassTemplateDecl* classTemplateDecl;
    public:
        ClassTemplateDeclSerializer(const ContextItems& contextItems, const ClassTemplateDecl* classTemplateDecl)
            : TemplateDeclBaseSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems)
            , classTemplateDecl(classTemplateDecl)
        {}
        std::string Serialize() const
        {
            return TemplateDeclBaseSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize(classTemplateDecl->getTemplateParameters(), classTemplateDecl->getTemplatedDecl());
        }
    };
}
