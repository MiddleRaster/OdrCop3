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
            QualType qualType(templateSpecializationType, 0);
            if (NeedsManualSerialization(contextItems, qualType))
            {
                      TemplateName  templateName = templateSpecializationType->getTemplateName();
                const TemplateDecl* templateDecl = templateName.getAsTemplateDecl();

                // if it's a TypeAliasTemplateDecl, desugar and serialize
                if (llvm::isa<clang::TypeAliasTemplateDecl>(templateDecl))
                    return IndentBlock(SerializeType(contextItems, QualType(templateSpecializationType, 0).getDesugaredType(contextItems.context)), 0);

                // for this test: "template<typename T> using Alias = Invisible<T>" where Invisible is defined in an anonymous namespace
                if (templateSpecializationType->isDependentType())
                    return SerializeDecl(contextItems, templateDecl);

                // e.g., for this test "Foo<Structural{42}> foo;"
                std::string out = templateDecl->getQualifiedNameAsString();
                out += "<";
                for (const clang::TemplateArgument& arg : templateSpecializationType->template_arguments())
                {
                    switch (arg.getKind())
                    {
                    case clang::TemplateArgument::Type       : out += TrimRightIf(IndentBlock(SerializeType(contextItems, arg.getAsType()), LengthOfLastLine(out)), ";"); break;
                    case clang::TemplateArgument::Expression : out += IndentBlock(SerializeExpr(contextItems, arg.getAsExpr()), LengthOfLastLine(out)); break;
                    case clang::TemplateArgument::Declaration:
                    {
                        if (const clang::Expr* expr = arg.getAsExpr())
                            out += IndentBlock(SerializeExpr(contextItems, expr), LengthOfLastLine(out));
                        else
                            out += arg.getAsDecl()->getQualifiedNameAsString();
                        break;
                    }
                    case clang::TemplateArgument::Template: out += arg.getAsTemplate().getAsTemplateDecl()->getNameAsString(); break;
                    case clang::TemplateArgument::Integral:
                    {
                        llvm::SmallString<32> str;
                        arg.getAsIntegral().toString(str, 10);
                        out += std::string(str);
                        break;
                    }
                    default:
                    {
                        std::string argStr;
                        llvm::raw_string_ostream os(argStr);
                        arg.print(contextItems.printPolicy, os, true);
                        os.flush();
                        out += argStr;
                        break;
                    }}
                    out += ", ";
                }
                out = TrimRightIf(out, ", ");
                out += ">";
                return out;
            }
            // else no manual serialization necessary
            
            // However, we might just need desugaring. 
            // Merely using qualType.print() does not work with:  "   TemplateUsingAliasToPointerToFunction<void, double, const char*, int, TemplateUsingAliasToPointerToFunction<int, float, double, const char*>, int, int> tuapfn;\n"
            // which is a template alias of a pointer to a function, where one of the arguments that same template alias of a pointer to a function.
            if (templateSpecializationType->isSugared())
                return IndentBlock(SerializeType(contextItems, QualType(templateSpecializationType, 0).getDesugaredType(contextItems.context)), 0);

            std::string str;
            llvm::raw_string_ostream os(str);
            qualType.print(os, contextItems.printPolicy);
            os.flush();
            return str + " " + contextItems.aux;
        }
    };
}