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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class RecordTypeSerializer
    {
        const ContextItems& contextItems;
        const RecordType  * recordType;
        QualType qt;
    public:
        RecordTypeSerializer(const ContextItems& contextItems, QualType qt, const RecordType* recordType) : contextItems(contextItems), qt(qt), recordType(recordType) {}

        std::string Serialize() const
        { // currently used for base types only. Will see how this unfolds in the future.

            if (recordType && recordType->getDecl()->isInAnonymousNamespace())
                return SerializeDecl(contextItems, recordType->getDecl());
            else
                return qt.getAsString(contextItems.printPolicy);
        }
    };
}