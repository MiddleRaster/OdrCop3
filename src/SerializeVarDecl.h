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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class VarDeclSerializer
    {
        const ContextItems& contextItems;
        const VarDecl     * varDecl;

        std::string get_TemplateHeader() const
        {   // this serializer may be called for varDecls that are actually varTemplateDecl; in that case, add template header
            if (const auto* varTemplateDecl = varDecl->getDescribedVarTemplate())
                return GetTemplateHeader<SerializeDecl, SerializeType, SerializeExpr>(contextItems, varTemplateDecl->getTemplateParameters());
            return "";
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
        std::string get_ConstexprAndInline() const { return varDecl->isConstexpr() ? "constexpr " : ""; }
        std::string get_Static()             const { return varDecl->isStaticDataMember() ? "static " : ""; }
        std::string get_Inline() const
        {
            std::string out;
            if (varDecl->isInline()) // constexpr is inline; don't print both
                if (false == varDecl->isConstexpr())
                    out += "inline ";
            return out;
        }
        
        const LambdaExpr* FindLambdaExpr(const Expr* expr) const
        {
            if (!expr)
                return nullptr;
            expr = expr->IgnoreImplicit();

            if (const auto* lambdaExpr = dyn_cast<LambdaExpr>(expr))
                return lambdaExpr;

            if (const auto* initListExpr = dyn_cast<InitListExpr>(expr))
                if (initListExpr->getNumInits() == 1)
                    return FindLambdaExpr(initListExpr->getInit(0));
                else 
                    return nullptr;

            if (const auto* constructExpr = dyn_cast<CXXConstructExpr>(expr))
                if (constructExpr->getNumArgs() == 1)
                    return FindLambdaExpr(constructExpr->getArg(0)->IgnoreImplicit());
                else
                    return nullptr;

            if (const auto* declRefExpr = dyn_cast<DeclRefExpr>(expr))
                if (const auto* referencedVar = dyn_cast<VarDecl>(declRefExpr->getDecl()))
                    if (referencedVar->hasInit())
                        return FindLambdaExpr(referencedVar->getInit()->IgnoreImplicit());

            return nullptr;
        }
        std::string get_RequiresClause(const LambdaExpr* lambdaExpr) const
        {
            const clang::CXXMethodDecl* lambdaCallOperator = nullptr;
            for (const clang::Decl* decl : lambdaExpr->getLambdaClass()->decls())
            {
                const clang::CXXMethodDecl* method;
                if (!(method = llvm::dyn_cast<clang::CXXMethodDecl>(decl)))
                    if (const auto* functionTemplate = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
                        method = llvm::dyn_cast<clang::CXXMethodDecl>(functionTemplate->getTemplatedDecl());
                if (method && method->getOverloadedOperator() == clang::OO_Call) {
                    lambdaCallOperator = method;
                    break;
                }
            }
            if (lambdaCallOperator)
            if (const auto& associatedConstraint = lambdaCallOperator->getTrailingRequiresClause())
            if (const clang::Expr* requiresExpr = associatedConstraint.ConstraintExpr)
            {
                std::string requiresStr;
                llvm::raw_string_ostream os(requiresStr);
                requiresExpr->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();
                return requiresStr;
            }
            return "";
        }
        std::string get_Init  () const
        {
            const Expr* expr = varDecl->getInit();
            if (!expr)
                return "";

            if (const auto* lambdaExpr = FindLambdaExpr(expr))
            {
                std::string body;
                llvm::raw_string_ostream os(body);
                lambdaExpr->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();

                auto pos = body.find("{");
                if (pos != std::string::npos)
                {
                    std::string requiresStr = get_RequiresClause(lambdaExpr); // insert requires clause, if necessary
                    std::string captureAndArgs = body.substr(0, pos);
                    body = captureAndArgs + (requiresStr == "" ? "" : "requires " + requiresStr + " ") + body.substr(pos);
                }

                if (isa<InitListExpr>(varDecl->getInit()->IgnoreImplicit()))
                    return "{"   + IndentBlock(body, 1) + "}";
                else
                    return " = " + IndentBlock(body, 3);
            }

            std::string e;
            llvm::raw_string_ostream os(e);
            expr->printPretty(os, nullptr, contextItems.printPolicy);
            os.flush();
            if (e == "")
                return "";
            if ((e.starts_with("{")) || e.starts_with("("))
                return e;
            else
                return " = " + e;
        }
    public:
        VarDeclSerializer(const ContextItems& contextItems, const VarDecl* varDecl) : contextItems(contextItems), varDecl(varDecl) {}
        std::string Serialize() const
        {
            std::string out;
            out += get_TemplateHeader();
            out += get_Attributes();
            out += get_Inline();
            out += get_Static();
            out += get_ConstexprAndInline();

            QualType type = varDecl->getType();
            if (varDecl->isConstexpr())
                type = type.withoutLocalFastQualifiers(); // constexpr vars are implicitly const. So strip off const.
            std::string name = varDecl->isOutOfLine() ? varDecl->getQualifiedNameAsString() : varDecl->getNameAsString();
            out += TrimRightIf(IndentBlock(SerializeType(contextItems, type), LengthOfLastLine(out)), " ");
            // snug up * and &
            if (!out.ends_with("*")) // e.g., "void *" gets no space
            if (!out.ends_with("&")) // e.g., ditto &
            if (!out.ends_with(" ")) // certainly don't want two spaces in a row
                out += " ";          // e.g., "int" does
            out += name;
            out += get_TemplateFooter();
            out += IndentBlock(get_Init(), LengthOfLastLine(out));
            out += ";\n";
            return out;
        }
    };
}