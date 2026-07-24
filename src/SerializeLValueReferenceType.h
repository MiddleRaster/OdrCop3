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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class LValueReferenceTypeSerializer
    {
        const ContextItems       & contextItems;
        const LValueReferenceType* lValueReferenceType;
        QualType qt;
    public:
        LValueReferenceTypeSerializer(const ContextItems& contextItems, QualType qt, const LValueReferenceType* lValueReferenceType) : contextItems(contextItems), qt(qt), lValueReferenceType(lValueReferenceType) {}

        std::string Serialize() const
        {
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);
            std::string out = IndentBlock(SerializeType(ci2, qt->getPointeeType()), 0);
            out = TrimRightIf(out, ";");
            return out + " &" + contextItems.aux;
        }
    };
}