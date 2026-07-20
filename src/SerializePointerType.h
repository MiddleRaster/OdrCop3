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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class PointerTypeSerializer
    {
        const ContextItems& contextItems;
        const PointerType * pointerType;
        QualType qt;
    public:
        PointerTypeSerializer(const ContextItems& contextItems, QualType qt, const PointerType* pointerType) : contextItems(contextItems), qt(qt), pointerType(pointerType) {}

        std::string Serialize() const
        {
            std::string out = SerializeType(contextItems, qt->getPointeeType());

            // pointers-to-functions have unusual syntax:  the "*" is not appended (in fact, it's already handled)
            if (qt->getPointeeType()->isFunctionProtoType())
                return out;

            // normal case
            return TrimRightIf(out, ";\n") + " *";
        }
    };
}