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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class ClassTemplateSpecializationDeclSerializer
    {
        const ContextItems& contextItems;
        const ClassTemplateSpecializationDecl* classTemplateSpecializationDecl;
    public:
        ClassTemplateSpecializationDeclSerializer(const ContextItems& contextItems, const ClassTemplateSpecializationDecl* classTemplateSpecializationDecl) : contextItems(contextItems), classTemplateSpecializationDecl(classTemplateSpecializationDecl) {}
        std::string Serialize() const
        {
            classTemplateSpecializationDecl->dump();

            return "this is where the ClassTemplateSpecializationDecl serialization happens";
        }
    };
}