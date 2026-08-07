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
#include "SerializeBinaryOperatorExpr.h"
#include "SerializeDeclRefExpr.h"
#include "SerializeConstantExpr.h"
#include "SerializeCXXFunctionalCastExpr.h"
#include "SerializeInitListExpr.h"
#include "SerializeUnaryExprOrTypeTraitExpr.h"
#include "SerializeParenExpr.h"
#include "SerializeCXXConstructExpr.h"
#include "SerializeImplicitCastExpr.h"

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
            static std::string Serialize(const ContextItems& contextItems, const clang::BinaryOperator           *            binaryOperator) { return        BinaryOperatorExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,            binaryOperator).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::UnaryExprOrTypeTraitExpr *  unaryExprOrTypeTraitExpr) { return  UnaryExprOrTypeTraitExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,  unaryExprOrTypeTraitExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::ParenExpr                *                 parenExpr) { return                 ParenExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,                 parenExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::CXXConstructExpr         *          cxxConstructExpr) { return          CXXConstructExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,          cxxConstructExpr).Serialize(); }
            static std::string Serialize(const ContextItems& contextItems, const clang::ImplicitCastExpr         *          implicitCastExpr) { return          ImplicitCastExprSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,          implicitCastExpr).Serialize(); }
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
            struct Can
            {
                static bool Print(const ContextItems& contextItems, const clang::Expr* expr)
                {
                    if (const clang::CXXConstructExpr* cxxContructExpr = dyn_cast<CXXConstructExpr>(expr))
                        for (unsigned i=0; i < cxxContructExpr->getNumArgs(); ++i)
                            if (Can::Print(contextItems, cxxContructExpr->getArg(i)) == false)
                                return false;

                    if (const clang::ImplicitCastExpr* implicitCastExpr = dyn_cast<ImplicitCastExpr>(expr))
                        if (Can::Print(contextItems, implicitCastExpr->getSubExpr()) == false)
                            return false;

                    if (const clang::DeclRefExpr* declRefExpr = dyn_cast<DeclRefExpr>(expr))
                        if (NeedsManualSerialization(contextItems, dyn_cast<clang::Decl>(declRefExpr->getDecl())) == true)
                            return false;

                    return !NeedsManualSerialization(contextItems, expr);
                }
            };
            if (Can::Print(contextItems, expr))
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
            case clang::Stmt::StmtClass::BinaryOperatorClass           : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::BinaryOperator           >(expr));
            case clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::UnaryExprOrTypeTraitExpr >(expr));
            case clang::Stmt::StmtClass::ParenExprClass                : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::ParenExpr                >(expr));
            case clang::Stmt::StmtClass::CXXConstructExprClass         : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::CXXConstructExpr         >(expr));
            case clang::Stmt::StmtClass::ImplicitCastExprClass         : return ExprSerializer::Serialize(contextItems, dyn_cast<clang::ImplicitCastExpr         >(expr));
            default:
                break;
            };
            expr->dump();
            throw OdrCop3::UnhandledException(std::string("unhandled Stmt::StmtClass: ") + enum_name<clang::Stmt::StmtClass,0,512>(expr->getStmtClass()));
        }
    }
}