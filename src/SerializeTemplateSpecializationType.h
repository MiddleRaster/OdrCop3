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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TemplateSpecializationTypeSerializer
    {
        const ContextItems              & contextItems;
        const TemplateSpecializationType* templateSpecializationType;
        QualType qt;
    public:
        TemplateSpecializationTypeSerializer(const ContextItems& contextItems, QualType qt, const TemplateSpecializationType* templateSpecializationType) : contextItems(contextItems), qt(qt), templateSpecializationType(templateSpecializationType) {}
        std::string Serialize() const
        {
            if (templateSpecializationType->isSugared())
                return IndentBlock(SerializeType(contextItems, QualType(templateSpecializationType, 0).getDesugaredType(contextItems.context)), 0);

            // for all other cases, just print
            QualType qualType(templateSpecializationType, 0);
            if (NeedsManualSerialization(contextItems, qualType))
            {
                // used to need this (when called from out this if clause); keeping it around for safekeeping only
                //if (const auto* recordType = templateSpecializationType->getAs<clang::RecordType>())
                //    return IndentBlock(SerializeType(contextItems, clang::QualType(recordType, 0)), 0);
                
                TemplateName  templateName = templateSpecializationType->getTemplateName();
                TemplateDecl* templateDecl = templateName.getAsTemplateDecl();
                std::string out = SerializeDecl(contextItems, templateDecl);
                return IndentBlock(out, 0);
            }
            std::string str;
            llvm::raw_string_ostream os(str);
            qualType.print(os, contextItems.printPolicy);
            os.flush();
            return str + " " + contextItems.aux;
        }
    };
}