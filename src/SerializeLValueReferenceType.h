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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class LValueReferenceTypeSerializer
    {
        const ContextItems       & contextItems;
        const LValueReferenceType* lValueReferenceType;
        QualType qt;
    public:
        LValueReferenceTypeSerializer(const ContextItems& contextItems, QualType qt, const LValueReferenceType* lValueReferenceType) : contextItems(contextItems), qt(qt), lValueReferenceType(lValueReferenceType) {}
        std::string Serialize() const
        {
            if (qt->getPointeeType()->isFunctionProtoType())
            {   // references-to-functions have atypical syntax
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, " (&" + contextItems.aux + ")"); // reference-to-function syntax
                std::string out = SerializeType(ci2, qt->getPointeeType());
                return out;
            }
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);
            std::string out;
            out += SerializeType(ci2, qt->getPointeeType());
            out  = TrimRightIf(out, "\n");
            out  = TrimRightIf(out, ";");
            out += " &" + contextItems.aux;
            return out;
        }
    };
}