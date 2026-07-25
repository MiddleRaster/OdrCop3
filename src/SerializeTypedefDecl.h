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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TypedefDeclSerializer
    {
        const ContextItems& contextItems;
        const TypedefDecl * typedefDecl;

        bool NeedsInlining(const TagDecl* tagDecl) const
        {
            if (tagDecl != nullptr)
            {
                if (tagDecl->getName().empty())
                    return true;

                const clang::DeclContext* declContext = tagDecl->getDeclContext();
                while (declContext && !declContext->isTranslationUnit())
                {
                    if (const auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
                        if (namespaceDecl->isAnonymousNamespace())
                            return true;
                    if (const auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(declContext))
                        if (recordDecl->isInAnonymousNamespace())
                            return true;
                    declContext = declContext->getParent();
                }
            }
            return false;
        }

    public:
        TypedefDeclSerializer(const ContextItems& contextItems, const TypedefDecl* typedefDecl) : contextItems(contextItems), typedefDecl(typedefDecl) {}

        std::string Serialize() const
        {
            QualType    underlying   = typedefDecl->getUnderlyingType().getCanonicalType();
            std::string aliasName    = typedefDecl->getQualifiedNameAsString();
            std::string resolvedType = underlying.getAsString(contextItems.printPolicy);

            const TagDecl* tagDecl = underlying->getAsTagDecl();
            if (NeedsInlining(tagDecl))
            {
                std::string fqtd    = "using " + aliasName + " = ";
                std::string inlined = TrimRightIf(SerializeDecl(contextItems, tagDecl), ";\n");
                fqtd += inlined;
                if (inlined.find('\n') == std::string::npos)
                    fqtd += "; // typedef " + inlined      + " " + aliasName + ";\n"; // not multi-line
                else
                    fqtd += "; // typedef " + resolvedType + " " + aliasName + ";\n"; // multi-line: just use the print result
                return fqtd;
            }
            return "using " + aliasName + " = " + resolvedType + "; // typedef " + resolvedType + " " + aliasName + ";\n";
        }
    };
}