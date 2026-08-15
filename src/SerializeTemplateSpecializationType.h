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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TemplateSpecializationTypeSerializer
    {
        const ContextItems              & contextItems;
        const TemplateSpecializationType* templateSpecializationType;
        QualType qt;
    public:
        TemplateSpecializationTypeSerializer(const ContextItems& contextItems, QualType qt, const TemplateSpecializationType* templateSpecializationType) : contextItems(contextItems), qt(qt), templateSpecializationType(templateSpecializationType) {}
        std::string Serialize() const
        {
                  TemplateName  templateName = templateSpecializationType->getTemplateName();
            const TemplateDecl* templateDecl = templateName.getAsTemplateDecl();

            // for this test: "template<typename T> using Alias = Invisible<T>" where Invisible is defined in an anonymous namespace
            if (templateSpecializationType->isDependentType())
                return SerializeDecl(contextItems, templateDecl);

            // e.g., for this test "Foo<Structural{42}> foo;"
            std::string out = templateDecl->getQualifiedNameAsString();
            out += "<";
            for (const clang::TemplateArgument& arg : templateSpecializationType->template_arguments())
            {
                out += SerializeTemplateArgument<SerializeDecl, SerializeType, SerializeExpr>(contextItems, arg, LengthOfLastLine(out));
                out += ", ";
            }
            out = TrimRightIf(out, ", ");
            out += ">";
            return out;
        }
    };
}