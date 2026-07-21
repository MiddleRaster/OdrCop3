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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class EnumTypeSerializer
    {
        const ContextItems& contextItems;
        const EnumType    * enumType;
        QualType qt;
    public:
        EnumTypeSerializer(const ContextItems& contextItems, QualType qt, const EnumType* enumType) : contextItems(contextItems), qt(qt), enumType(enumType) {}

        std::string Serialize() const
        {
            std::string out;
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);
            out += SerializeDecl(ci2, enumType->getDecl());
            out  = TrimRightIf(out, ";\n");
            out += " " + contextItems.aux;
            return out;
        }
    };
}