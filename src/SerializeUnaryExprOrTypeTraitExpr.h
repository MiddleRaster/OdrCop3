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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class UnaryExprOrTypeTraitExprSerializer
    {
        const ContextItems            & contextItems;
        const UnaryExprOrTypeTraitExpr* unaryExprOrTypeTraitExpr;
    public:
        UnaryExprOrTypeTraitExprSerializer(const ContextItems& contextItems, const UnaryExprOrTypeTraitExpr* unaryExprOrTypeTraitExpr) : contextItems(contextItems), unaryExprOrTypeTraitExpr(unaryExprOrTypeTraitExpr) {}
        std::string Serialize() const
        {
            std::string out;

            switch (unaryExprOrTypeTraitExpr->getKind())
            {
            case UnaryExprOrTypeTrait::UETT_AlignOf: out += "alignof"; break;
            case UnaryExprOrTypeTrait::UETT_SizeOf : out += "sizeof" ; break;
            case UnaryExprOrTypeTrait::UETT_CountOf: out += "countof"; break;
            case UnaryExprOrTypeTrait::UETT_DataSizeOf:
            case UnaryExprOrTypeTrait::UETT_Last:
            case UnaryExprOrTypeTrait::UETT_OpenMPRequiredSimdAlign:
            case UnaryExprOrTypeTrait::UETT_PreferredAlignOf:
            case UnaryExprOrTypeTrait::UETT_PtrAuthTypeDiscriminator:
            case UnaryExprOrTypeTrait::UETT_VecStep:
            default: 
                out += std::string(getTraitName(unaryExprOrTypeTraitExpr->getKind()));
                break;
            }
            if (unaryExprOrTypeTraitExpr->isArgumentType())
                out += "(" + IndentBlock(SerializeType(contextItems, unaryExprOrTypeTraitExpr->getArgumentType()), LengthOfLastLine(out)+1) + ")";
            else
                out += "(" + IndentBlock(SerializeExpr(contextItems, unaryExprOrTypeTraitExpr->getArgumentExpr()), LengthOfLastLine(out)+1) + ")";
            return out;
        }
    };
}