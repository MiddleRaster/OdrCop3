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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class BinaryOperatorExprSerializer
    {
        const ContextItems & contextItems;
        const BinaryOperator* binaryOperator;

        std::string get_Side(const Expr* expr, size_t indent) const
        {
            if (NeedsManualSerialization(contextItems, expr))
                return IndentBlock(SerializeExpr(contextItems, expr), indent);

            std::string str;
            llvm::raw_string_ostream os(str);
            expr->printPretty(os, nullptr, contextItems.printPolicy);
            os.flush();
            return str;
        }

    public:
        BinaryOperatorExprSerializer(const ContextItems& contextItems, const BinaryOperator* binaryOperator) : contextItems(contextItems), binaryOperator(binaryOperator) {}
        std::string Serialize() const
        {
            std::string out;
            out += get_Side(binaryOperator->getLHS(), LengthOfLastLine(out)); // LHS
            out += " " + binaryOperator->getOpcodeStr().str() + " ";          // operator:  e.g. "+", "-", "*", "/", "=="
            out += get_Side(binaryOperator->getRHS(), LengthOfLastLine(out)); // RHS
            return out;
        }
    };
}