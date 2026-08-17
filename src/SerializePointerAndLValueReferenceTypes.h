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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class PointerAndLValueReferenceTypesSerializer
    {
        const ContextItems& contextItems;
        QualType qt;

        bool IsEventuallyArrayOrFunctionProtoType(QualType qt) const
        {
            if (const auto* pointerType = qt->getAs<PointerType>())
                return IsEventuallyArrayOrFunctionProtoType(pointerType->getPointeeType());

            if (const auto* referenceType = qt->getAs<ReferenceType>())
                return IsEventuallyArrayOrFunctionProtoType(referenceType->getPointeeType());

            if (qt->isArrayType())
                return true;
            if (qt->isFunctionProtoType())
                return true;

            return false;
        }

    public:
        PointerAndLValueReferenceTypesSerializer(const ContextItems& contextItems, QualType qt) : contextItems(contextItems), qt(qt) {}
        std::string Serialize(const std::string& starOrAmpersand) const
        {
            bool pointerToFunctionOrArraySyntax = IsEventuallyArrayOrFunctionProtoType(qt->getPointeeType());
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls,  pointerToFunctionOrArraySyntax ? starOrAmpersand + contextItems.aux : "");
            std::string out;
            out = SerializeType(ci2, qt->getPointeeType());
            out = TrimRightIf(out, "\n");
            out = TrimRightIf(out, ";");
            if (true == pointerToFunctionOrArraySyntax)
                return out; // name is already added

            // add name (and pointer/ampersand)
            out += SnugUpPointersAndReferences(out); // add space only as appropriate:  we want "Foo **&foo", for example
            out += starOrAmpersand + contextItems.aux;
            return out;
        }
    };
}