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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class DecayedTypeSerializer
    {
        const ContextItems& contextItems;
        const DecayedType * decayedType;
        QualType qt;
    public:
        DecayedTypeSerializer(const ContextItems& contextItems, QualType qt, const DecayedType* decayedType) : contextItems(contextItems), qt(qt), decayedType(decayedType) {}

        std::string Serialize() const
        {
            return SerializeType(contextItems, decayedType->getOriginalType());
        }
    };
}