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

#include <exception>
#include <string>
#include <sstream>
#include <set>

namespace OdrCop3
{
    struct ContextItems
    {
        ASTContext& context;
        const PrintingPolicy& printPolicy;
        const std::string& TU;
        std::unordered_set<const Decl*>& recursingDecls;
        std::string aux;
        bool wantFunctionBody = true;
        bool needsFriend      = false;
        ContextItems(ASTContext* context, const PrintingPolicy& policy, const std::string& TU, std::unordered_set<const Decl*>& recursingDecls, const std::string& aux="")
            : context       (*context)
            , printPolicy   (policy)
            , TU            (TU)
            , recursingDecls(recursingDecls)
            , aux           (aux)
        {}
    };

	struct UnhandledException : public std::exception
	{
		const std::string message;
        UnhandledException(const std::string& msg) : message(msg) {}
		const char* what() const noexcept override { return message.c_str(); }
	};

    inline std::string IndentBlock(const std::string& block, size_t indentWidth, const std::string& firstLinePrefix = "")
    {
        std::istringstream iss(block);
        std::string indentation(indentWidth, ' ');
        std::string out;
        bool first = true;
        for (std::string line; std::getline(iss, line);)
        {
            if (first) {
                first = false;
                out  += firstLinePrefix + line + "\n";
            } else
                out  += indentation + line + "\n";
        }
        return out.substr(0, out.size()-1); // always remove last "\n"
    }

    inline std::string MakeUnnamedAndAnonymousConsistent(std::string input)
    {
        struct Replace
        {
            static std::string With(std::string str, const std::string& bad, const std::string& good)
            {
                auto pos = str.find(bad);
                if (pos != std::string::npos)
                    str.replace(pos, bad.size(), good);
                return str;
            }
        };

        input = Replace::With(input, "(unnamed at"         , "(anonymous type at");
        input = Replace::With(input, "(unnamed enum at"    , "(anonymous type at");
        input = Replace::With(input, "(unnamed union at"   , "(anonymous type at");
        input = Replace::With(input, "(anonymous struct at", "(anonymous type at");
        input = Replace::With(input, "(anonymous class at" , "(anonymous type at");
        input = Replace::With(input, "(anonymous union at" , "(anonymous type at");

        return input;
    }

    inline const clang::Decl* StripPointersAndReferences(const clang::Decl* decl)
    {
        clang::QualType qualType;
             if (const auto* vd = llvm::dyn_cast<clang::ValueDecl>(decl)) qualType = vd->getType();
        else if (const auto* td = llvm::dyn_cast<clang::TypeDecl >(decl)) qualType = td->getTypeForDecl()->getCanonicalTypeInternal();
        else
            return nullptr; // no associated type → cannot be "defined" in anon ns via type
        qualType = qualType.getCanonicalType();

        for (;;)
        {
            if (qualType->isPointerType()         || 
                qualType->isReferenceType()       ||
                qualType->isMemberPointerType()   ||
                qualType->isFunctionPointerType() || 
                qualType->isFunctionReferenceType())
            {
                qualType = qualType->getPointeeType();
                continue;
            }
            if (qualType->isArrayType())
            {
                qualType = clang::QualType(qualType->getArrayElementTypeNoTypeQual(), 0);
                continue;
            }
            break;
        }

        const clang::Decl* typeDecl = nullptr;
             if (const auto* rt  = qualType->getAs<clang::RecordType >())              typeDecl = rt->getDecl();
        else if (const auto* et  = qualType->getAs<clang::EnumType   >())              typeDecl = et->getDecl();
        else if (const auto* tt  = qualType->getAs<clang::TypedefType>())              typeDecl = tt->getDecl();
        else if (const auto* tst = qualType->getAs<clang::TemplateSpecializationType>())
             if (clang::TemplateDecl* td = tst->getTemplateName().getAsTemplateDecl()) typeDecl = td;

        return typeDecl;
    }
    inline bool IsDefinedInAnonymousNamespace(const clang::DeclContext* declContext)
    {
        while (declContext != nullptr)
        {
            if (const auto* ns = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
                if (ns->isAnonymousNamespace())
                    return true;

            declContext = declContext->getParent();
        }
        return false;
    }
    inline bool IsDefinedInAnonymousNamespace(const clang::Decl* decl)
    {
        if (true == IsDefinedInAnonymousNamespace(decl->getDeclContext()))
            return true;
        if (const clang::Decl* typeDecl = StripPointersAndReferences(decl))
            return IsDefinedInAnonymousNamespace(typeDecl->getDeclContext());
        return false; // builtin/dependent/no defining Decl => not in anonymous namespace
    }
    
    inline bool ContainsAnonymousType(clang::QualType qualType)
    {
        qualType = qualType.getCanonicalType();

        if (const auto* recordType = qualType->getAs<clang::RecordType>())
            return IsDefinedInAnonymousNamespace(static_cast<const Decl*>(recordType->getDecl()));

        if (const auto* enumType = qualType->getAs<clang::EnumType>())
            return IsDefinedInAnonymousNamespace(static_cast<const Decl*>(enumType->getDecl()));

        if (qualType->isPointerType() || qualType->isReferenceType())
            return ContainsAnonymousType(qualType->getPointeeType());

        if (const auto* memberPointerType = qualType->getAs<clang::MemberPointerType>())
        {
            if (const clang::CXXRecordDecl* classDecl = memberPointerType->getMostRecentCXXRecordDecl())
                if (true == IsDefinedInAnonymousNamespace(static_cast<const Decl*>(classDecl)))
                    return true;
            return ContainsAnonymousType(memberPointerType->getPointeeType());
        }

        if (qualType->isArrayType())
            return ContainsAnonymousType(clang::QualType(qualType->getArrayElementTypeNoTypeQual(), 0));

        if (const auto* fnProtoType = qualType->getAs<clang::FunctionProtoType>())
        {
            if (true == ContainsAnonymousType(fnProtoType->getReturnType()))
                return true;
            for (clang::QualType paramType : fnProtoType->getParamTypes())
                if (true == ContainsAnonymousType(paramType))
                    return true;
        }
        return false;
    }

}