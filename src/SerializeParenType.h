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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class ParenTypeSerializer
    {
        const ContextItems& contextItems;
        const ParenType   * parenType;
        QualType qt;
    public:
        ParenTypeSerializer(const ContextItems& contextItems, QualType qt, const ParenType* parenType) : contextItems(contextItems), qt(qt), parenType(parenType) {}

        std::string Serialize() const
        {
            if (const auto* functionProtoType = parenType->getInnerType()->getAs<clang::FunctionProtoType>())
            {   // pointers to functions get unusual syntax
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, " (*)");
                // use new ContextItems if aux is not already in use; else use existing one
                return SerializeType(contextItems.aux == "" ? ci2: contextItems, parenType->getInnerType());
            }
            return "(" + SerializeType(contextItems, parenType->getInnerType()) + ")";
        }
    };
}