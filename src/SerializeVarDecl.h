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

    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class VarDeclSerializer
    {
        const ContextItems& contextItems;
        const VarDecl     * varDecl;
    public:
        VarDeclSerializer(const ContextItems& contextItems, const VarDecl* varDecl) : contextItems(contextItems), varDecl(varDecl) {}

        std::string Serialize() const
        {
            std::string out;

            for (const auto* attr : varDecl->attrs())
                out += SerializeAttr(contextItems, attr); // attributes on static data-members

            if (varDecl->isConstexpr())
                out += "constexpr ";
            else if (varDecl->isInline()) // constexpr is inline; don't print both
                out += "inline ";

            if (varDecl->isStaticDataMember())
                out += "static ";

            {
                std::string fieldStr;
                llvm::raw_string_ostream os(fieldStr);
                varDecl->getType().print(os, contextItems.printPolicy, varDecl->getNameAsString());
                os.flush();
                out += fieldStr;
            }

            // if it's a VarTemplateSpecializationDecl, add <whatever> after the name
            if (const auto* varTemplateSpecializationDecl = llvm::dyn_cast<clang::VarTemplateSpecializationDecl>(varDecl))
            {
                out += TemplateArgsToString<SerializeDecl, SerializeType, SerializeAttr>(contextItems, varTemplateSpecializationDecl->getTemplateArgs(), varTemplateSpecializationDecl->getSpecializedTemplate()->getTemplateParameters());
            }

            if (varDecl->hasInit())
            {
                const Expr* expr = varDecl->getInit();
                llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts());
                std::string      init = text.str();
                if ((init.starts_with("{")) || init.starts_with("("))
                    out += init;
                else
                    out += "=" + init;
            }
            out += ";\n";
            return out;
        }
    };
}