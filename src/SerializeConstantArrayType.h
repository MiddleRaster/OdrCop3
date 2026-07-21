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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class ConstantArrayTypeSerializer
    {
        const ContextItems     & contextItems;
        const ConstantArrayType* constantArrayType;
        QualType qt;
    public:
        ConstantArrayTypeSerializer(const ContextItems& contextItems, QualType qt, const ConstantArrayType* constantArrayType) : contextItems(contextItems), qt(qt), constantArrayType(constantArrayType) {}

        std::string Serialize() const
        {
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);
            std::string out = IndentBlock(SerializeType(ci2, constantArrayType->getElementType()), 0);
            out  = TrimRightIf(out, ";");
            out += (contextItems.aux.size() > 0 ? " " : "") + contextItems.aux + "[" + std::to_string(constantArrayType->getSize().getZExtValue()) + "]";
            return out;
        }
    };
}