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
#include "magic_enum.h"

#include "SerializeConceptSpecializationExpr.h"
#include "SerializeUnaryOperatorExpr.h"
#include "SerializeDeclRefExpr.h"
#include "SerializeConstantExpr.h"
#include "SerializeCXXFunctionalCastExpr.h"
#include "SerializeInitListExpr.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr>
        struct Expr
        {
            static std::string Serialize(const ContextItems& contextItems, const clang::ConceptSpecializationExpr* conceptSpecializationExpr) { return ConceptSpecializationExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, conceptSpecializationExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::UnaryOperator            *             unaryOperator) { return         UnaryOperatorExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,             unaryOperator).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::DeclRefExpr              *               declRefExpr) { return               DeclRefExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,               declRefExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::ConstantExpr             *              constantExpr) { return              ConstantExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,              constantExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::CXXFunctionalCastExpr    *     cXXFunctionalCastExpr) { return     CXXFunctionalCastExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,     cXXFunctionalCastExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::InitListExpr             *              initListExpr) { return              InitListExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,              initListExpr).Serialize(); }
        };

        template <auto SerializeDecl, auto SerializeType>
        std::string SerializeExpr_Save(const ContextItems& contextItems, const clang::Expr* expr)
        {
            if (!expr)
                return {};

            if (!NeedsManualSerialization(contextItems, expr))
            {
                std::string exprStr;
                llvm::raw_string_ostream os(exprStr);
                expr->printPretty(os, nullptr, contextItems.printPolicy);
                return exprStr;
            }
            // else do it manually

            if (const auto* integerLiteral = llvm::dyn_cast<clang::IntegerLiteral>(expr))
            {
                llvm::SmallString<32> str;
                integerLiteral->getValue().toString(str, 10, true);
                return std::string(str);
            }

            if (const auto* boolLiteral = llvm::dyn_cast<clang::CXXBoolLiteralExpr>(expr))
                return boolLiteral->getValue() ? "true" : "false";

            if (const auto* parenExpr = llvm::dyn_cast<clang::ParenExpr>(expr))
                return "(" + SerializeExpr<SerializeDecl, SerializeType>(contextItems, parenExpr->getSubExpr()) + ")";

            {
                std::string exprStr;
                llvm::raw_string_ostream os(exprStr);
                expr->printPretty(os, nullptr, contextItems.printPolicy);
                return exprStr;
            }
        }

        template<auto SerializeDecl, auto SerializeType>
        static std::string Exprs(const ContextItems& contextItems, const clang::Expr* expr)
        {
            if (!NeedsManualSerialization(contextItems, expr))
            {
                std::string exprStr;
                llvm::raw_string_ostream os(exprStr);
                expr->printPretty(os, nullptr, contextItems.printPolicy);
                return exprStr;
            }

            using ExprSerializer = Serialize::Expr<SerializeDecl, SerializeType, &Exprs<SerializeDecl, SerializeType>>;
            switch (expr->getStmtClass())
            {
            case clang::Stmt::StmtClass::ConceptSpecializationExprClass: return ExprSerializer::Serialize(contextItems, dyn_cast<clang::ConceptSpecializationExpr>(expr));
            case clang::Stmt::StmtClass::UnaryOperatorClass            : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::UnaryOperator            >(expr));
            case clang::Stmt::StmtClass::DeclRefExprClass              : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::DeclRefExpr              >(expr));
            case clang::Stmt::StmtClass::ConstantExprClass             : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::ConstantExpr             >(expr));
            case clang::Stmt::StmtClass::CXXFunctionalCastExprClass    : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::CXXFunctionalCastExpr    >(expr));
            case clang::Stmt::StmtClass::InitListExprClass             : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::InitListExpr             >(expr));
            default:
                break;
            };
            throw OdrCop3::UnhandledException(std::string("unhandled Stmt::StmtClass: ") + enum_name<clang::Stmt::StmtClass,0,512>(expr->getStmtClass()));
        }
    }
}