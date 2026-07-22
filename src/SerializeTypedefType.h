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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TypedefTypeSerializer
    {
        const ContextItems& contextItems;
        const TypedefType * typedefType;
        QualType qt;
    public:
        TypedefTypeSerializer(const ContextItems& contextItems, QualType qt, const TypedefType* typedefType) : contextItems(contextItems), qt(qt), typedefType(typedefType) {}

        std::string Serialize() const
        {   // for an ODR violations detector, we never want any typedefs or using aliases. Just keep desugaring.
            return SerializeType(contextItems, typedefType->desugar());
        }
    };
}