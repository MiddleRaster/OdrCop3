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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class DependentSizedArrayTypeSerializer
    {
        const ContextItems           & contextItems;
        const DependentSizedArrayType* dependentSizedArrayType;
        QualType qt;
    public:
        DependentSizedArrayTypeSerializer(const ContextItems& contextItems, QualType qt, const DependentSizedArrayType* dependentSizedArrayType) : contextItems(contextItems), qt(qt), dependentSizedArrayType(dependentSizedArrayType) {}

        std::string Serialize() const
        {
            std::string out;

            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);
            out += IndentBlock(SerializeType(ci2, dependentSizedArrayType->getElementType()), 0);
            out += " " + contextItems.aux;
            out += "[";
            {
                std::string result;
                llvm::raw_string_ostream os(result);
                dependentSizedArrayType->getSizeExpr()->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();
                out += result;
            }
            out += "];\n";
            return out;
        }
    };
}