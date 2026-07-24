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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class ClassTemplateDeclSerializer
    {
        const ContextItems     & contextItems;
        const ClassTemplateDecl* classTemplateDecl;
    public:
        ClassTemplateDeclSerializer(const ContextItems& contextItems, const ClassTemplateDecl* classTemplateDecl) : contextItems(contextItems), classTemplateDecl(classTemplateDecl) {}
        std::string Serialize() const
        {
            std::string out = ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeAttr>(contextItems, classTemplateDecl->getTemplateParameters());
            out += IndentBlock(SerializeDecl(contextItems, classTemplateDecl->getTemplatedDecl()), LengthOfLastLine(out));
            return out + "\n";
        }
    };
}
