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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TypedefTypeSerializer
    {
        const ContextItems& contextItems;
        const TypedefType * typedefType;
        QualType qt;
    public:
        TypedefTypeSerializer(const ContextItems& contextItems, QualType qt, const TypedefType* typedefType) : contextItems(contextItems), qt(qt), typedefType(typedefType) {}
        // for an ODR violations detector, we never want any typedefs or using aliases. Just keep desugaring.
        std::string Serialize() const { return SerializeDecl(contextItems, typedefType->getDecl()); }
    };
}