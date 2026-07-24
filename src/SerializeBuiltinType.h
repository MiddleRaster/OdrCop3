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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class BuiltinTypeSerializer
    {
        const ContextItems& contextItems;
        const BuiltinType * builtinType;
        QualType qt;
    public:
        BuiltinTypeSerializer(const ContextItems& contextItems, QualType qt, const BuiltinType* builtinType) : contextItems(contextItems), qt(qt), builtinType(builtinType) {}

        std::string Serialize() const
        {
            return qt.getAsString(contextItems.printPolicy);
        }
    };
}