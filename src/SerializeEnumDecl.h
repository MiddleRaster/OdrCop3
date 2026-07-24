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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> inline std::string BuildFullyQualifiedParentChain(const ContextItems& contextItems, const clang::EnumDecl* enumDecl)
    {
        std::string parent;
        const clang::DeclContext* declContext = enumDecl->getDeclContext();
        while (declContext && !declContext->isTranslationUnit())
        {
            if (const auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(declContext))
            {
                std::string name;
                if (const ClassTemplateSpecializationDecl * ctsd = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl))
                    name = ctsd->getNameAsString() + TemplateArgsToString<SerializeDecl, SerializeType, SerializeAttr>(contextItems, ctsd, false); // include template instantiations
                else
                    name = recordDecl->getNameAsString();
                parent = name + "::" + parent;
            }
            else if (const auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
            {
                parent = namespaceDecl->getNameAsString() + "::" + parent;
            }
            declContext = declContext->getParent();
        }
        return parent;
    }
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> inline std::string BuildNameForNameless(const ContextItems& contextItems, const clang::EnumDecl* enumDecl)
    {
        std::string namelessName;

        // is it the C-like syntax case?
        if (const TypedefNameDecl* typedefNameDecl = enumDecl->getTypedefNameForAnonDecl())
        {
            clang::SourceManager& sourceManager = enumDecl->getASTContext().getSourceManager();
            clang::PresumedLoc      presumedLoc = sourceManager.getPresumedLoc(enumDecl->getLocation());
            namelessName = std::string("(anonymous type at ") + presumedLoc.getFilename() + ":" + std::to_string(presumedLoc.getLine()) + ":" + std::to_string(presumedLoc.getColumn()) + ")";
        }
        else
        {
            std::string enumStr;
            llvm::raw_string_ostream os(enumStr);
            QualType enumQT = enumDecl->getASTContext().getTagType(ElaboratedTypeKeyword::None, /*Qualifier=*/std::nullopt, enumDecl, /*OwnsTag=*/false);
            enumQT.print(os, contextItems.printPolicy, enumDecl->getNameAsString());
            os.flush();
            namelessName = OdrCop3::MakeUnnamedAndAnonymousConsistent(enumStr);
        }

        // add scoping if any
        std::string anonymous = "(anonymous type at ";
        auto pos = namelessName.find(anonymous);
        if (pos != std::string::npos)
            namelessName.insert(pos, BuildFullyQualifiedParentChain<SerializeDecl, SerializeType, SerializeAttr>(contextItems, enumDecl));

        return namelessName;
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> inline std::string MakeUnnamedEnumKey(const ContextItems& contextItems, const clang::EnumDecl* enumDecl)
    {
        std::string name = BuildNameForNameless<SerializeDecl, SerializeType, SerializeAttr>(contextItems, enumDecl);
        name = TrimRightIf(name, ")");

        std::string firstEnumName;
        if (!enumDecl->enumerators().empty())
            firstEnumName = enumDecl->enumerators().begin()->getName().str();
        else
            firstEnumName = "<empty>";

        name += " " + firstEnumName + ")";
        return name;
    };

    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class EnumDeclSerializer
    {
        const ContextItems & contextItems;
        const EnumDecl     * enumDecl;

        std::string Print() const
        {
            std::string str;
        
            clang::PrintingPolicy policy = contextItems.printPolicy;
            policy.FullyQualifiedName    = true;
            llvm::raw_string_ostream os(str);
            enumDecl->print(os, policy);
            os.flush();

            // modify str to look more like my output:

            // change 4 spaces to 3.
            std::size_t pos  = 0;
            std::string from = "\n    ";
            std::string to   = " ";
            while ((pos = str.find(from, pos)) != std::string::npos)
                str.replace(pos, from.size(), to);

            // remove spaces around =
            pos  = 0;
            from = " = ";
            to   = "=";
            while ((pos = str.find(from, pos)) != std::string::npos)
                str.replace(pos, from.size(), to);

            // remove all "\n"
            pos = 0;
            while ((pos = str.find('\n', pos)) != std::string::npos)
                str.erase(pos, 1);

            str  = TrimRightIf(str, "}");
            str += " };\n";
            return str;
        }
    public:
        EnumDeclSerializer(const ContextItems& contextItems, const EnumDecl* enumDecl) : contextItems(contextItems), enumDecl(enumDecl) {}

        std::string Serialize() const
        {
            std::string enumName = enumDecl->getNameAsString();
            if ((enumName != "") && !enumDecl->isInAnonymousNamespace())
                return Print(); // not anonymous type nor defined in anonymous namespace: do shortcut.

            if (enumName == "")
                enumName = BuildNameForNameless<SerializeDecl, SerializeType, SerializeAttr>(contextItems, enumDecl);
            else
                enumName = enumDecl->getQualifiedNameAsString();
            
            std::string fqe = (enumDecl->isScoped() ? "enum class " : "enum ") + enumName + (enumDecl->isFixed() ? " : " + enumDecl->getIntegerType().getCanonicalType().getAsString() : "") + " { ";
            bool first = true;
            for (const EnumConstantDecl* enumeratorDecl : enumDecl->enumerators())
            {
                if (first)
                    first = false;
                else
                    fqe += ", ";
                
                fqe += enumeratorDecl->getName();
                if (const Expr* Init = enumeratorDecl->getInitExpr())
                {
                    llvm::APSInt value = enumeratorDecl->getInitVal();
                    Expr::EvalResult Result;
                    if (Init->EvaluateAsInt(Result, contextItems.context) && (value == Result.Val.getInt()))
                        fqe += "=" + llvm::toString(value, 10); // Clang's semantic value is valid.
                    else
                        fqe += "=" + Lexer::getSourceText(CharSourceRange::getTokenRange(Init->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts()).str(); // go with what the user typed
                }
            }
            return fqe + " };\n";
        }
    };
}