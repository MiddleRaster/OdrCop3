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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TypedefDeclSerializer
    {
        const ContextItems& contextItems;
        const TypedefDecl * typedefDecl;
        bool NeedsInlining(QualType underlying) const
        {
            if (const TagDecl* tagDecl = underlying->getAsTagDecl(); tagDecl != nullptr)
                if (tagDecl->getName().empty())
                    return true;
            return OdrCop3::NeedsManualSerialization(contextItems, underlying);
        }
    public:
        TypedefDeclSerializer(const ContextItems& contextItems, const TypedefDecl* typedefDecl) : contextItems(contextItems), typedefDecl(typedefDecl) {}
        std::string Serialize() const
        {
            std::string fqtd;
            if (true == contextItems.aux.empty())
            {
                fqtd += "typedef ";
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, typedefDecl->getNameAsString());
                fqtd += IndentBlock(SerializeType(ci2,          typedefDecl->getUnderlyingType().getCanonicalType()), LengthOfLastLine(fqtd));
            } else
                fqtd += IndentBlock(SerializeType(contextItems, typedefDecl->getUnderlyingType().getCanonicalType()), LengthOfLastLine(fqtd));
            fqtd  = TrimRightIf(fqtd, " "); // for enums
            fqtd  = TrimRightIf(fqtd, ";"); // for UDTs
            return fqtd + ";\n";
        }
    };
}