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
            QualType    underlying   = typedefDecl->getUnderlyingType().getCanonicalType();
            std::string aliasName    = typedefDecl->getQualifiedNameAsString();
            std::string resolvedType = underlying.getAsString(contextItems.printPolicy);

            if (NeedsInlining(underlying)) {
                std::string fqtd     = "using " + aliasName + " = ";
                std::string inlined  = SerializeType(contextItems, underlying);
                inlined              = TrimRightIf(inlined, " "); // for enums
                inlined              = TrimRightIf(inlined, ";"); // for UDTs
                fqtd += inlined;
                if (inlined.find('\n') == std::string::npos)
                    fqtd += "; // typedef " + inlined      + " " + aliasName + ";\n"; // not multi-line: use serialized result
                else
                    fqtd += "; // typedef " + resolvedType + " " + aliasName + ";\n"; // multi-line: just use the print result
                return fqtd;
            }
            return "using " + aliasName + " = " + resolvedType + "; // typedef " + resolvedType + " " + aliasName + ";\n";
        }
    };
}