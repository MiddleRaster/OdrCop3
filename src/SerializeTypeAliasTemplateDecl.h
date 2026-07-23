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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TypeAliasTemplateDeclSerializer
    {
        const ContextItems         & contextItems;
        const TypeAliasTemplateDecl* typeAliasTemplateDecl;

        void PrintParam(const NamedDecl* param, std::string& out) const
        {
            if (auto* TTPP = llvm::dyn_cast<TemplateTemplateParmDecl>(param)) {
                out += "template <";

                bool innerFirst = true;
                for (auto* inner : *TTPP->getTemplateParameters()) {
                    if (!innerFirst)
                        out += ", ";
                    innerFirst = false;
                    PrintParam(inner, out); // recursion for nested template-template parameters
                }

                out += "> class";
                if (TTPP->isParameterPack())
                    out += "...";
                out += " ";
                out += TTPP->getNameAsString();
                return;
            }

            if (auto* TTP = llvm::dyn_cast<TemplateTypeParmDecl>(param)) {
                out += "typename";
                if (TTP->isParameterPack())
                    out += "...";
                out += " ";
                out += TTP->getNameAsString();
                return;
            }

            if (auto* NTTP = llvm::dyn_cast<NonTypeTemplateParmDecl>(param)) {
                out += NTTP->getType().getAsString();
                if (NTTP->isParameterPack())
                    out += "...";
                out += " ";
                out += NTTP->getNameAsString();
                return;
            }
        };

    public:
        TypeAliasTemplateDeclSerializer(const ContextItems& contextItems, const TypeAliasTemplateDecl* typeAliasTemplateDecl) : contextItems(contextItems), typeAliasTemplateDecl(typeAliasTemplateDecl) {}

        std::string Serialize() const
        {
        //  typeAliasTemplateDecl->dump();

            std::string params;
            bool first = true;
            for (const NamedDecl* param : *typeAliasTemplateDecl->getTemplateParameters())
            {
                if (first)
                    first = false;
                else
                    params += ", ";
                PrintParam(param, params);
            }

            TypeAliasDecl* aliasDecl = typeAliasTemplateDecl->getTemplatedDecl();
            std::string    aliasName = typeAliasTemplateDecl->getQualifiedNameAsString();
            std::string         fqtd = "template<" + params + "> using " + aliasName + " = ";
            QualType      underlying = aliasDecl->getUnderlyingType();
            if (NeedsManualSerialization(contextItems, underlying))
            {
                fqtd += IndentBlock(SerializeType(contextItems, underlying), fqtd.size());
                fqtd  = TrimRightIf(fqtd, ";");
            } else
                fqtd += underlying.getAsString(contextItems.printPolicy);

            fqtd += "; // no typedef equivalent\n";
            return fqtd;
        }
    };
}