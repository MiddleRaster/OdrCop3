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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class LambdaExprSerializer
    {
        const ContextItems& contextItems;
        const LambdaExpr  * lambdaExpr;
    public:
        LambdaExprSerializer(const ContextItems& contextItems, const LambdaExpr* lambdaExpr) : contextItems(contextItems), lambdaExpr(lambdaExpr) {}
        std::string Serialize() const
        {
            std::string body;
            llvm::raw_string_ostream os(body);

            // first the capture stuff
            std::string out;
            out += "[";
            switch (lambdaExpr->getCaptureDefault())
            {
            default:
            case LCD_None:                     break;
            case LCD_ByCopy: out += "=";    break;
            case LCD_ByRef:  out += "&";    break;
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
                        out += ", ";

                    if (capture.isPackExpansion())
                        out += "...";

                    if (capture.capturesThis())
                    {
                        if (capture.getCaptureKind() == LCK_StarThis)
                            out += "*this";
                        else
                            out += "this";
                    }
                    else if (lambdaExpr->isInitCapture(&capture))
                    {
                        out += capture.getCapturedVar()->getNameAsString();
                        out += " = ";
                        out += IndentBlock(SerializeExpr(contextItems, *init), LengthOfLastLine(out));
                    }
                    else if (capture.capturesVariable())
                    {
                        if (capture.isPackExpansion())
                            out += "...";
                        else if (capture.getCaptureKind() == LCK_ByRef)
                            out += "&";
                        out += capture.getCapturedVar()->getNameAsString();
                    }
                }
                ++init;
            }
            out += "]";

            // then the explicit template parameters
            if (const TemplateParameterList* templateParameters = lambdaExpr->getTemplateParameterList();
                templateParameters && templateParameters->getLAngleLoc().isValid()) // if any
            {
                out += "<";
                for (unsigned index=0; index<templateParameters->size(); ++index)
                {
                    if (index != 0)
                        out += ", ";
                    out += TrimRightIf(IndentBlock(SerializeDecl(contextItems, templateParameters->getParam(index)), LengthOfLastLine(out)), ";");
                }
                out += ">";
            }

            // then the parameters
            const CXXMethodDecl* callOperator = lambdaExpr->getCallOperator();
            out += "(";
            for (unsigned index=0; index<callOperator->getNumParams(); ++index)
            {
                if (index != 0)
                    out += ", ";

                out += TrimRightIf(IndentBlock(SerializeDecl(contextItems, callOperator->getParamDecl(index)), LengthOfLastLine(out)), ";");
            }
            out += ")";

            if (lambdaExpr->isMutable())
                out += " mutable";

            // then the explicit return type
            if (lambdaExpr->hasExplicitResultType())
            {
                out += " -> ";
                out += IndentBlock(SerializeType(contextItems, callOperator->getReturnType()), LengthOfLastLine(out));
            }

            // then the trailing requires-clause
            const AssociatedConstraint& trailingRequires = lambdaExpr->getTrailingRequiresClause();
            if (trailingRequires.ConstraintExpr != nullptr)
            {
                out += " requires ";
                out += IndentBlock(SerializeExpr(contextItems, trailingRequires.ConstraintExpr), LengthOfLastLine(out));
            }

            // finally the body
            out += " ";
            lambdaExpr->getBody()->printPretty(os, nullptr, contextItems.printPolicy);
            os.flush(); // result is in "body"

            out += IndentBlock(body, 0) + "\n";
            out += InternalLinkageReferenceCollector::PrintReferences<SerializeDecl>(lambdaExpr->getBody(), contextItems);

            return out;
        }
    };
}