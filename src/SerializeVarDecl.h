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

        std::string get_TemplateHeader() const
        {   // this serializer may be called for varDecls that are actually varTemplateDecl; in that case, add template header
            const auto* varTemplateDecl = varDecl->getDescribedVarTemplate();
            return (varTemplateDecl == nullptr) ? "" : ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeAttr>(contextItems, varTemplateDecl->getTemplateParameters());
        }
        std::string get_TemplateFooter() const
        {   // if it's a VarTemplateSpecializationDecl, add <whatever> after the name
            const auto* varTemplateSpecializationDecl = llvm::dyn_cast<clang::VarTemplateSpecializationDecl>(varDecl);
            return varTemplateSpecializationDecl == nullptr ? "" : TemplateArgsToString(contextItems, varTemplateSpecializationDecl);
        }
        std::string get_Attributes() const
        {
            std::string out;
            for (const auto* attr : varDecl->attrs())
                out += SerializeAttr(contextItems, attr);
            return out;
        }
        std::string get_ConstexprAndInline() const
        {
            std::string out;
            if (varDecl->isConstexpr())
                out += "constexpr ";
            else if (varDecl->isInline()) // constexpr is inline; don't print both
                out += "inline ";
            return out;
        }
        std::string get_Static() const { return varDecl->isStaticDataMember() ? "static " : ""; }
        std::string get_Init() const
        {
            if (!varDecl->hasInit()) return "";

            const Expr* expr = varDecl->getInit();
            llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts());
            std::string      init = text.str();
            if ((init.starts_with("{")) || init.starts_with("("))
                return init;
            else
                return "=" + init;
        }

    public:
        VarDeclSerializer(const ContextItems& contextItems, const VarDecl* varDecl) : contextItems(contextItems), varDecl(varDecl) {}

        std::string Serialize() const
        {
            std::string out;
            out += get_TemplateHeader();
            out += get_Attributes();
            out += get_ConstexprAndInline();
            out += get_Static();

            std::string name = varDecl->isOutOfLine() ? varDecl->getQualifiedNameAsString() : varDecl->getNameAsString();
            if (NeedsManualSerialization(contextItems, varDecl->getType())) {
                out += IndentBlock(SerializeType(contextItems, varDecl->getType()), LengthOfLastLine(out));
                out += name;
            } else {
                std::string varStr;
                llvm::raw_string_ostream os(varStr);
                varDecl->getType().print(os, contextItems.printPolicy, name);
                os.flush();
                out += varStr;
            }
            out += get_TemplateFooter();
            out += get_Init();
            out += ";\n";
            return out;
        }
    };
}