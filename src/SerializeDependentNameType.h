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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class DependentNameTypeSerializer
    {
        const ContextItems     & contextItems;
        const DependentNameType* dependentNameType;
        QualType qt;
    public:
        DependentNameTypeSerializer(const ContextItems& contextItems, QualType qt, const DependentNameType* dependentNameType) : contextItems(contextItems), qt(qt), dependentNameType(dependentNameType) {}

        std::string Serialize() const
        {
            std::string result;
            llvm::raw_string_ostream os(result);
            qt.print(os, contextItems.printPolicy);
            os.flush();
            return result;
        }
    };
}