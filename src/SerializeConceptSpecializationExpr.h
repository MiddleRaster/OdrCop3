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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ConceptSpecializationExprSerializer
    {
        const ContextItems             & contextItems;
        const ConceptSpecializationExpr* conceptSpecializationExpr;
    public:
        ConceptSpecializationExprSerializer(const ContextItems& contextItems, const ConceptSpecializationExpr* conceptSpecializationExpr) : contextItems(contextItems), conceptSpecializationExpr(conceptSpecializationExpr) {}
        std::string Serialize() const
        {
            std::string out;
            out += conceptSpecializationExpr->getNamedConcept()->getNameAsString();
            out += "<";

            for (const TemplateArgumentLoc& argLoc : conceptSpecializationExpr->getTemplateArgsAsWritten()->arguments())
            {
                const TemplateArgument& arg = argLoc.getArgument();
                switch (arg.getKind())
                {
                case clang::TemplateArgument::Type: out += TrimRightIf(IndentBlock(SerializeType(contextItems, arg.getAsType()), LengthOfLastLine(out)), ";"); break;
                case clang::TemplateArgument::Expression: out += IndentBlock(SerializeExpr(contextItems, arg.getAsExpr()), LengthOfLastLine(out)); break;
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
                }
                }
                out += ", ";
            }
            out = TrimRightIf(out, ", ");
            out += "> ";

            return out;
        }
    };
}