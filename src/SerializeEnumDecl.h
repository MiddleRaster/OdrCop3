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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> inline std::string BuildFullyQualifiedParentChain(const ContextItems& contextItems, const clang::EnumDecl* enumDecl)
    {
        std::string parent;
        const clang::DeclContext* declContext = enumDecl->getDeclContext();
        while (declContext && !declContext->isTranslationUnit())
        {
            std::string name;
            if (const auto* namespaceDecl = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
                name = namespaceDecl->getNameAsString();
            else
            if (const auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(declContext))
            {
                if (const ClassTemplateSpecializationDecl * ctsd = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(recordDecl))
                    name = ctsd->getNameAsString() + TemplateArgsToString(contextItems, ctsd); // include template instantiations
                else
                    name = recordDecl->getNameAsString();
            }
            parent = name + "::" + parent;
            declContext = declContext->getParent();
        }
        return parent;
    }
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> inline std::string BuildNameForNameless(const ContextItems& contextItems, const clang::EnumDecl* enumDecl)
    {

        // is it the C-like syntax case?
        if (const TypedefNameDecl* typedefNameDecl = enumDecl->getTypedefNameForAnonDecl())
        {
            clang::SourceManager& sourceManager = enumDecl->getASTContext().getSourceManager();
            clang::PresumedLoc      presumedLoc = sourceManager.getPresumedLoc(enumDecl->getLocation());
            std::string namelessName = std::string("(unnamed enum at ") + presumedLoc.getFilename() + ":" + std::to_string(presumedLoc.getLine()) + ":" + std::to_string(presumedLoc.getColumn()) + ")";
            return namelessName;
        }
        else
        {
            std::string enumStr;
            llvm::raw_string_ostream os(enumStr);
            QualType enumQT = enumDecl->getASTContext().getTagType(ElaboratedTypeKeyword::None, /*Qualifier=*/std::nullopt, enumDecl, /*OwnsTag=*/false);
            enumQT.print(os, contextItems.printPolicy, enumDecl->getNameAsString());
            os.flush();
            return enumStr;
        }
    }
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> inline std::string MakeUnnamedEnumKey(const ContextItems& contextItems, const clang::EnumDecl* enumDecl)
    {
        std::string name = BuildNameForNameless<SerializeDecl, SerializeType, SerializeExpr>(contextItems, enumDecl);
        name = TrimRightIf(name, ")");

        std::string firstEnumName;
        if (!enumDecl->enumerators().empty())
            firstEnumName = enumDecl->enumerators().begin()->getName().str();
        else
            firstEnumName = "<empty>";

        name += " " + firstEnumName + ")";
        return name;
    };

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class EnumDeclSerializer
    {
        const ContextItems & contextItems;
        const EnumDecl     * enumDecl;

        std::string Print() const
        {
            std::string str;
            llvm::raw_string_ostream os(str);
            enumDecl->print(os, contextItems.printPolicy);
            os.flush();
            return str + ";\n";
        }
    public:
        EnumDeclSerializer(const ContextItems& contextItems, const EnumDecl* enumDecl) : contextItems(contextItems), enumDecl(enumDecl) {}
        std::string Serialize() const
        {
            std::string enumName = enumDecl->getNameAsString();
            if ((enumName != "") && !enumDecl->isInAnonymousNamespace())
                return Print(); // not anonymous type nor defined in anonymous namespace: do shortcut.

            if (enumName == "")
                enumName = BuildNameForNameless<SerializeDecl, SerializeType, SerializeExpr>(contextItems, enumDecl);
            else
                enumName = enumDecl->getQualifiedNameAsString();
            
            std::string fqe = (enumDecl->isScoped() ? "enum class " : "enum ") + enumName + (enumDecl->isFixed() ? " : " + enumDecl->getIntegerType().getCanonicalType().getAsString() : "") + " {\n";
            for (const EnumConstantDecl* enumeratorDecl : enumDecl->enumerators())
            {
                fqe += "    " + enumeratorDecl->getName().str();
                if (const Expr* Init = enumeratorDecl->getInitExpr())
                {
                    llvm::APSInt value = enumeratorDecl->getInitVal();
                    Expr::EvalResult Result;
                    if (Init->EvaluateAsInt(Result, contextItems.context) && (value == Result.Val.getInt()))
                        fqe += " = " + llvm::toString(value, 10); // Clang's semantic value is valid.
                    else
                        fqe += " = " + Lexer::getSourceText(CharSourceRange::getTokenRange(Init->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts()).str(); // go with what the user typed
                }
                fqe += ",\n";
            }
            fqe = TrimRightIf(fqe, ",\n");
            return fqe + "\n};\n";
        }
    };
}