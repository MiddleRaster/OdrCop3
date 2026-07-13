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

#include <vector>

#include "SerializationUtils.h"

namespace OdrCop3
{
    inline std::string MakeUnnamedEnumKey(const clang::EnumDecl* enumDecl)
    {
        // 1. Build the fully qualified parent chain
        std::string parent;
        const clang::DeclContext* declContext = enumDecl->getDeclContext();
        while (declContext && !declContext->isTranslationUnit())
        {
            if (const auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(declContext))
            {
                std::string name;
                //  if (const auto* ctsd = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl))
                //     name = ctsd->getNameAsString() + TemplateArgsToString(ctsd); // include template instantiations
                //  else
                name = recordDecl->getNameAsString();
                parent = name + "::" + parent;
            }
            else if (const auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
            {
                if (!namespaceDecl->isAnonymousNamespace())
                    parent = namespaceDecl->getNameAsString() + "::" + parent;
            }
            declContext = declContext->getParent();
        }

        // 2. Extract the first enumerator name
        std::string firstEnumName;
        if (!enumDecl->enumerators().empty())
            firstEnumName = enumDecl->enumerators().begin()->getName().str();
        else
            firstEnumName = "<empty>";

        // 3. Construct the final key
        return parent + "(unnamed enum: " + firstEnumName + ")";
    };

    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class EnumDeclSerializer
    {
        const ContextItems & contextItems;
        const EnumDecl     * enumDecl;
    public:
        EnumDeclSerializer(const ContextItems& contextItems, const EnumDecl* enumDecl) : contextItems(contextItems), enumDecl(enumDecl) {}

        std::string Serialize() const
        {
            std::string underlyingType = enumDecl->getIntegerType().getCanonicalType().getAsString();
            bool              isScoped = enumDecl->isScoped();   // enum class vs enum
            bool               isFixed = enumDecl->isFixed();
            std::string       enumName = enumDecl->getNameAsString();
            std::string     prettyEnum;
            if (enumName == "")
            {
                if (const TypedefNameDecl* typedefNameDecl = enumDecl->getTypedefNameForAnonDecl())
                {
                    clang::SourceManager& sourceManager = enumDecl->getASTContext().getSourceManager();
                    clang::PresumedLoc    presumedLoc   = sourceManager.getPresumedLoc(enumDecl->getLocation());
                    prettyEnum = std::string("(anonymous type at ") + presumedLoc.getFilename() + ":" + std::to_string(presumedLoc.getLine()) + ":" + std::to_string(presumedLoc.getColumn()) + ")";
                }
                else
                {
                    std::string enumStr;
                    llvm::raw_string_ostream os(enumStr);
                    QualType enumQT = enumDecl->getASTContext().getTagType(ElaboratedTypeKeyword::None, /*Qualifier=*/std::nullopt, enumDecl, /*OwnsTag=*/false);
                    enumQT.print(os, contextItems.printPolicy, enumDecl->getNameAsString());
                    os.flush();
                    prettyEnum = OdrCop3::MakeUnnamedAndAnonymousConsistent(enumStr);
                }

                std::string anonymous = "(anonymous type at ";
                auto pos = prettyEnum.find(anonymous);
                if (pos != std::string::npos)
                    if (auto* parentDecl = llvm::dyn_cast<clang::NamedDecl>(enumDecl->getDeclContext()))
                        prettyEnum.insert(pos, parentDecl->getQualifiedNameAsString() + "::");
            } else
                prettyEnum = enumDecl->getQualifiedNameAsString();
            
            std::string fqe = (isScoped ? "enum class " : "enum ") + prettyEnum + (isFixed ? " : " + underlyingType : "") + " { ";
            for (const clang::EnumConstantDecl* enumeratorDecl : enumDecl->enumerators())
            {
                std::string enumeratorName = enumeratorDecl->getName().str();
                std::string val = llvm::toString(enumeratorDecl->getInitVal(), 10);
                fqe += enumeratorName + "=" + val + ", ";
            }
            fqe = fqe.substr(0, fqe.size()-2) + " };\n";
            return fqe;
        }
    };
}