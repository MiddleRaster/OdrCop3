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
                out += Serialize::Attrs<SerializeDecl, SerializeType, SerializeExpr>(contextItems, attr);
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
                if (!contextItems.serializationNeeds.hasInternalLinkageRef &&
                    !contextItems.serializationNeeds.hasNamespaceAlias)
                {
                    lambdaExpr->printPretty(os, nullptr, contextItems.printPolicy);
                    os.flush();

                    auto pos = body.find("{");
                    if (pos != std::string::npos)
                    {
                        std::string requiresStr = get_RequiresClause(lambdaExpr); // insert requires clause, if necessary
                        std::string captureAndArgs = body.substr(0, pos);
                        body = captureAndArgs + (requiresStr == "" ? "" : "requires " + requiresStr + " ") + body.substr(pos);
                    }
                } else {

                    // first the capture stuff
                    std::string lambda;
                    lambda += "[";
                    switch (lambdaExpr->getCaptureDefault())
                    {
                    default:
                    case LCD_None:                     break;
                    case LCD_ByCopy: lambda += "=";    break;
                    case LCD_ByRef:  lambda += "&";    break;
                    }
                    bool firstCapture = lambdaExpr->getCaptureDefault() == LCD_None;
                    auto init = lambdaExpr->capture_init_begin();
                    for (const LambdaCapture& capture : lambdaExpr->captures())
                    {
                        if (!capture.isImplicit())
                        {
                            if (firstCapture)
                                firstCapture = false;
                            else
                                lambda += ", ";

                            if (capture.isPackExpansion())
                                lambda += "...";

                            if (capture.capturesThis())
                            {
                                if (capture.getCaptureKind() == LCK_StarThis)
                                    lambda += "*this";
                                else
                                    lambda += "this";
                            }
                            else if (lambdaExpr->isInitCapture(&capture))
                            {
                                lambda += capture.getCapturedVar()->getNameAsString();
                                lambda += " = ";
                                lambda += IndentBlock(SerializeExpr(contextItems, *init), LengthOfLastLine(lambda));
                            }
                            else if (capture.capturesVariable())
                            {
                                if (capture.getCaptureKind() == LCK_ByRef)
                                    lambda += "&";
                                lambda += capture.getCapturedVar()->getNameAsString();
                            }
                        }
                        ++init;
                    }
                    lambda += "]";

                    // then the explicit template parameters
                    if (const TemplateParameterList* templateParameters = lambdaExpr->getTemplateParameterList();
                        templateParameters && templateParameters->getLAngleLoc().isValid()) // if any
                    {
                        lambda += "<";
                        for (unsigned index=0; index<templateParameters->size(); ++index)
                        {
                            if (index != 0)
                                lambda += ", ";
                            lambda += TrimRightIf(IndentBlock(SerializeDecl(contextItems, templateParameters->getParam(index)), LengthOfLastLine(lambda)), ";");
                        }
                        lambda += ">";
                    }

                    // then the parameters
                    const CXXMethodDecl* callOperator = lambdaExpr->getCallOperator();
                    lambda += "(";
                    for (unsigned index = 0; index < callOperator->getNumParams(); ++index)
                    {
                        if (index != 0)
                            lambda += ", ";

                        lambda += TrimRightIf(IndentBlock(SerializeDecl(contextItems, callOperator->getParamDecl(index)), LengthOfLastLine(lambda)), ";");
                    }
                    lambda += ")";

                    if (lambdaExpr->isMutable())
                        lambda += " mutable";

                    // then the explicit return type
                    if (lambdaExpr->hasExplicitResultType())
                    {
                        lambda += " -> ";
                        lambda += IndentBlock(SerializeType(contextItems, callOperator->getReturnType()), LengthOfLastLine(lambda));
                    }

                    // then the trailing requires-clause
                    const AssociatedConstraint& trailingRequires = lambdaExpr->getTrailingRequiresClause();
                    if (trailingRequires.ConstraintExpr != nullptr)
                    {
                        lambda += " requires ";
                        lambda += IndentBlock(SerializeExpr(contextItems, trailingRequires.ConstraintExpr), LengthOfLastLine(lambda));
                    }

                    // finally the body
                    lambda += " ";
                    lambdaExpr->getBody()->printPretty(os, nullptr, contextItems.printPolicy);
                    os.flush(); // result is in "body"

                    lambda += IndentBlock(body, 0);
                    lambda += InternalLinkageReferenceCollector::PrintReferences<SerializeDecl>(lambdaExpr->getBody(), contextItems);

                    body = lambda;
                }

                if (isa<InitListExpr>(varDecl->getInit()->IgnoreImplicit()))
                    return "{"   + IndentBlock(body, 1) + "}";
                else
                    return " = " + IndentBlock(body, 3);
            }

            std::string initStr;
            if (contextItems.serializationNeeds.AreAllFalse())
            {
                llvm::raw_string_ostream os(initStr);
                expr->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();
                if (initStr == "")
                    return "";
            } else {
                if (const auto* ctorExpr = llvm::dyn_cast<clang::CXXConstructExpr>(expr->IgnoreImplicit()); ctorExpr && ctorExpr->getParenOrBraceRange().isInvalid())
                    return ""; // implicit default-construction, nothing written — initStr.g. "Foo x;" via a typedef chain
                initStr = IndentBlock(SerializeExpr(contextItems, expr), 0);
            }
            switch (varDecl->getInitStyle())
            {
            default:
            case clang::VarDecl::InitializationStyle::ListInit: return                     initStr;
            case clang::VarDecl::InitializationStyle::CInit   : return " = " + IndentBlock(initStr, 3);
            case clang::VarDecl::InitializationStyle::CallInit: return   "(" + IndentBlock(initStr, 1) + ")";
            }
        }

        std::string GetInlineStaticConstAndConstexpr() const
        {   // no longer duplicating what DeclPrinter::VisitVarDecl(VarDecl *D) does exactly (notably, "inline" is ignored completely), as this would miss valid ODR violations

            std::string out;
            StorageClass storageClass = varDecl->getStorageClass();
            switch(storageClass)
            {
            case StorageClass::SC_None  :
            default                     :                   break;
            case StorageClass::SC_Extern: out += "extern "; break;
            case StorageClass::SC_Static: out += "static "; break;
            }
            switch(varDecl->getTSCSpec())
            {
            case ThreadStorageClassSpecifier::TSCS_unspecified :
            default                                            :                         break;
            case ThreadStorageClassSpecifier::TSCS_thread_local: out += "thread_local "; break;
            }
            if (varDecl->isInlineSpecified())
                out += "inline ";
            if (varDecl->isConstexpr())
                out += "constexpr "; // there is code to strip off const in Serialize(), just like DeclPrinter does
            return out;
        }

    public:
        VarDeclSerializer(const ContextItems& contextItems, const VarDecl* varDecl) : contextItems(contextItems), varDecl(varDecl) {}
        std::string Serialize() const
        {
            std::string  name = varDecl->isOutOfLine()                            ? varDecl->getQualifiedNameAsString()   : varDecl->getNameAsString();
            QualType qualType = contextItems.serializationNeeds.hasNamespaceAlias ? varDecl->getType().getCanonicalType() : varDecl->getType();
            if (varDecl->isConstexpr())
                qualType = qualType.withoutLocalFastQualifiers(); // constexpr vars are implicitly const. So strip off const. Just like DeclPrinter does.

            std::string out;
            out += get_TemplateHeader();
            out += get_Attributes();
            out += GetInlineStaticConstAndConstexpr();
            std::string aux = IsType::EventuallyArrayOrFunctionPointer(qualType) ? name : contextItems.aux;
            out += TrimRightIf(IndentBlock(SerializeType(contextItems.withAux(aux), qualType), LengthOfLastLine(out)), " ");
            if (true == aux.empty())
            {
                out += SnugUpPointersAndReferences(out);
                out += name;
                out += get_TemplateFooter();
            }
            out += IndentBlock(get_Init(), LengthOfLastLine(out));
            out += ";\n";
            return out;
        }
    };
}