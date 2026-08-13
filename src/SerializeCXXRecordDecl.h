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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class CXXRecordDeclSerializer
    {
        const ContextItems & contextItems;
        const CXXRecordDecl* cxxRecordDecl;

        std::string get_Kind()        const { return cxxRecordDecl->getKindName().str() + " "; }
        std::string get_Friend()      const { return contextItems.needsFriend ? "friend " : ""; }
        std::string get_Name()        const
        {
            // is it the C-like syntax case?
            if (const TypedefNameDecl* typedefNameDecl = cxxRecordDecl->getTypedefNameForAnonDecl())
            {   // similar to enum case. // TODO: REVIEW: should this be combined with enum case?
                clang::SourceManager& sourceManager = cxxRecordDecl->getASTContext().getSourceManager();
                clang::PresumedLoc      presumedLoc = sourceManager.getPresumedLoc(cxxRecordDecl->getLocation());
                std::string namelessName = std::string("(unnamed ") + get_Kind() + "at " + presumedLoc.getFilename() + ":" + std::to_string(presumedLoc.getLine()) + ":" + std::to_string(presumedLoc.getColumn()) + ")";
                return namelessName;
            }

            std::string fullyQualifiedName = cxxRecordDecl->getQualifiedNameAsString();
            if((fullyQualifiedName.find("(anonymous struct at ") != std::string::npos) ||
               (fullyQualifiedName.find("(anonymous union at " ) != std::string::npos) ||
               (fullyQualifiedName.find("(anonymous class at " ) != std::string::npos))
                return ""; // just like print() does

            // leaving these here, because we'll probably need them for (anonymous namespace) stuff
            if (fullyQualifiedName.find("(anonymous ") != std::string::npos)
                return MakeUnnamedAndAnonymousConsistent(fullyQualifiedName);
            if (fullyQualifiedName.find("(unnamed ") != std::string::npos)
            {
                const auto pos = fullyQualifiedName.rfind("::");
                if (pos != std::string::npos)
                    fullyQualifiedName.erase(0, pos + 2);
                return fullyQualifiedName;
            }
            return cxxRecordDecl->getNameAsString();
        }
        std::string get_Attributes(bool* hasFinal) const
        {
            std::string out;
            for (const Attr* attr : cxxRecordDecl->attrs()) // alignas/[[attributes]]/__declspecs
            {
                std::string a = SerializeAttr(contextItems, attr);
                if (a == "final ")
                    *hasFinal = true;
                else
                    out += a;
            }
            return out;
        }
        std::string get_Bases() const
        {
            std::string out;

            bool firstBase = true;
            for (const clang::CXXBaseSpecifier& base : cxxRecordDecl->bases())
            {
                if (firstBase) {
                    firstBase = false;
                    out += ": ";
                } else
                    out += ", ";

                // do public/protected/private as well as virtual (exactly what the user wrote, which may NOT be what decl::print prints
                const clang::SourceManager& sourceManager = contextItems.context.getSourceManager();
                const clang::SourceLocation         begin = sourceManager.getSpellingLoc(base.getBeginLoc());
                const clang::SourceLocation baseTypeBegin = sourceManager.getSpellingLoc(base.getBaseTypeLoc());
                const clang::CharSourceRange        range = clang::CharSourceRange::getCharRange(begin, baseTypeBegin);
                llvm::StringRef prefix = clang::Lexer::getSourceText(range, sourceManager, contextItems.context.getLangOpts()).trim();
                if (!prefix.empty())
                {
                    out += prefix;
                    out += ' ';
                }

                out += IndentBlock(SerializeType(contextItems, base.getType()), LengthOfLastLine(out));
                out  = TrimRightIf(out, ";");
            }
            if (firstBase == false)
                out += " ";

            out += "{";
            return out;
        }
        static bool IsNestedAnonymousAndHasOwner(const Decl* decl)
        {
            // unnamed UDT
            if (const auto* nestedRecord = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
            if (nestedRecord->getName().empty())
            for (const Decl* decl : nestedRecord->getParent()->decls())
            {
                if (decl->isImplicit())
                    continue;

                if (auto* valueDecl = dyn_cast<ValueDecl>(decl))
                    if (valueDecl->getType()->getAsCXXRecordDecl() == nestedRecord)
                        return true;

                if (auto* typedefDecl = dyn_cast<TypedefNameDecl>(decl))
                    if (typedefDecl->getUnderlyingType()->getAsCXXRecordDecl() == nestedRecord)
                        return true;
            }

            // unnamed enum
            if (const auto* nestedEnum = llvm::dyn_cast<clang::EnumDecl>(decl))
            if (nestedEnum->getName().empty())
            for (const Decl* decl : nestedEnum->getParent()->decls())
            {
                if (decl->isImplicit())
                    continue;

                if (const auto* valueDecl = dyn_cast<ValueDecl>(decl))
                    if (const auto* enumType = valueDecl->getType()->getAs<EnumType>())
                        if (enumType->getDecl() == nestedEnum)
                            return true;

                if (const auto* typedefDecl = dyn_cast<TypedefNameDecl>(decl))
                    if (const auto* enumType = typedefDecl->getUnderlyingType()->getAs<EnumType>())
                        if (enumType->getDecl() == nestedEnum)
                            return true;
            }

            return false;
        }

    public:
        CXXRecordDeclSerializer(const ContextItems& contextItems, const CXXRecordDecl* cxxRecordDecl) : contextItems(contextItems), cxxRecordDecl(cxxRecordDecl) {}
        std::string Serialize() const
        {
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);

            std::string out;
            out += get_Friend();
            out += get_Kind(); // struct/class/union keyword
            bool hasFinal = false; // final is treated as an attribute, but it's really a keyword
            out += get_Attributes(&hasFinal);
            out += get_Name() + contextItems.aux; // aux contains <args>
            if (!cxxRecordDecl->isThisDeclarationADefinition())
                return out + ";\n"; // if it's a declaration, go no farther
            out = TrimRightIf(out, " "); // certainly don't want two spaces in a row, which happens if it's a nameless class/struct/untion
            out += " ";
            if (hasFinal) // final is treated as an attribute, but it's really a keyword
                out += "final ";
            out += IndentBlock(get_Bases(), LengthOfLastLine(out));
            out += "\n";
            for (const clang::Decl* decl : cxxRecordDecl->decls())
            {   // data-members, methods, nested decls, etc.
                if (decl->isImplicit())
                    continue;

                if (IsNestedAnonymousAndHasOwner(decl))
                    continue; // Decl::print() combines an unnamed union with field, rather than output two Decls.

                if (decl->getKind() == clang::Decl::Kind::AccessSpec)
                    out += SerializeDecl(ci2, decl); // "public:", for instance, does not get indented
                else
                    out += IndentBlock(SerializeDecl(ci2, decl), 4, "    ") + "\n";
            }
            out += "};\n";
            return out;
        }
    };
}