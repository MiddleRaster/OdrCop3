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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ClassTemplateDeclSerializer
    {
        const ContextItems     & contextItems;
        const ClassTemplateDecl* classTemplateDecl;

        std::string get_TemplateHeader() const
        {
            const auto* templateParameterList =    classTemplateDecl->getTemplateParameters();

            std::string templatePrefix;
            if (NeedsManualSerialization(contextItems, templateParameterList))
                templatePrefix = IndentBlock(ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeExpr>(contextItems, templateParameterList), 0);
            else
            {
                llvm::raw_string_ostream os(templatePrefix);
                templateParameterList->print(os, contextItems.context, contextItems.printPolicy);
                os.flush();
            }

            // change "template <" to "template<"
            std::string::size_type pos = templatePrefix.find("template <");
            if (pos != std::string::npos)
                templatePrefix.replace(pos, 10, "template<");
            return templatePrefix;
        }

    public:
        ClassTemplateDeclSerializer(const ContextItems& contextItems, const ClassTemplateDecl* classTemplateDecl) : contextItems(contextItems), classTemplateDecl(classTemplateDecl) {}
        std::string Serialize() const
        {
            std::string out = get_TemplateHeader();
            return out + IndentBlock(SerializeDecl(contextItems, classTemplateDecl->getTemplatedDecl()), LengthOfLastLine(out)) + "\n";
        }
    };
}
