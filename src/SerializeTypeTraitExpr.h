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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TypeTraitExprSerializer
    {
        const ContextItems & contextItems;
        const TypeTraitExpr* typeTraitExpr;
    public:
        TypeTraitExprSerializer(const ContextItems& contextItems, const TypeTraitExpr* typeTraitExpr) : contextItems(contextItems), typeTraitExpr(typeTraitExpr) {}
        std::string Serialize() const
        {
            TypeTrait Trait = typeTraitExpr->getTrait();
            std::string out = getTraitSpelling(Trait);
            out += "(";

            for (unsigned i=0; i<typeTraitExpr->getNumArgs(); ++i)
                out += IndentBlock(SerializeType(contextItems, typeTraitExpr->getArg(i)->getType()), LengthOfLastLine(out)) + ", ";

            out = TrimRightIf(out, ", ");
            out += ")";
            return out;
        }
    };
}