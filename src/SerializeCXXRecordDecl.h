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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class CXXRecordDeclSerializer
    {
        const ContextItems & contextItems;
        const CXXRecordDecl* cxxRecordDecl;
    public:
        CXXRecordDeclSerializer(const ContextItems& contextItems, const CXXRecordDecl* cxxRecordDecl) : contextItems(contextItems), cxxRecordDecl(cxxRecordDecl) {}

        std::string get_Kind()        const { return cxxRecordDecl->getKindName().str() + " "; }
        std::string get_Name()        const { return MakeUnnamedAndAnonymousConsistent(cxxRecordDecl->getQualifiedNameAsString()); }
        std::string get_SizeComment() const { return cxxRecordDecl->isCompleteDefinition() && !cxxRecordDecl->isDependentType() ? " // sizeof=" + std::to_string(contextItems.context.getASTRecordLayout(cxxRecordDecl).getSize().getQuantity()) + "\n" : "\n"; }

        std::string get_Attributes(bool* hasFinal) const
        {
            std::string out;
            for (const Attr* attr : cxxRecordDecl->attrs()) // alignas/[[attributes]]/__declspecs
            {
                std::string a = SerializeAttr(contextItems, attr);
                if (a == "final ")
                    *hasFinal = true;
                else
                    out += SerializeAttr(contextItems, attr);
            }
            return out;
        }
        std::string get_Bases() const
        {
            std::string out;

            bool firstBase = true;
            for (const clang::CXXBaseSpecifier& base : cxxRecordDecl->bases())
            {   // CXXBaseSpecifier is not a Decl or a Type:  must serialize some stuff here
                if (firstBase) {
                    firstBase = false;
                    out += ": ";
                } else
                    out += ", ";

                switch (base.getAccessSpecifier()) {
                case clang::AS_public:    out += "public ";    break;
                case clang::AS_protected: out += "protected "; break;
                case clang::AS_private:   out += "private ";   break;
                case clang::AS_none:
                default:                                       break;
                }

                if (base.isVirtual())
                    out += "virtual ";

                out += IndentBlock(SerializeType(contextItems, base.getType()), out.size() - (out.rfind('\n')+1));
                out  = out.substr(0, out.size()-1); // IndentBlock adds '\n' to every line
                if (out.ends_with(";"))
                    out = out.substr(0, out.size()-1); // if it's an anonymous namespace type, we put in the full definition which ends in ";"
            }
            if (firstBase == false)
                out += " ";

            out += "{";
            return out;
        }

        std::string Serialize() const
        {
            std::string out;

            if (cxxRecordDecl->isInjectedClassName())
                return out;
            if (cxxRecordDecl->isLambda())
                return out;

            // template
            if (const clang::ClassTemplateDecl* ctd = cxxRecordDecl->getDescribedClassTemplate())
                out += ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeAttr>(contextItems, ctd->getTemplateParameters());
            else if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(cxxRecordDecl))
                out += "template<> ";
            
            out += get_Kind(); // struct/class/union keyword

            bool hasFinal = false; // final is treated as an attribute, but it's really a keyword
            out += get_Attributes(&hasFinal);
            out += get_Name(); // NOTE: no " " until after template stuff

            // if it's a template instantiation, add <arg> 
            if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(cxxRecordDecl))
            {
                out += IndentBlock(TemplateArgsToString<SerializeDecl, SerializeType, SerializeAttr>(contextItems, CTSD), out.size());
                out  = out.substr(0, out.size()-1); // strip off last "\n"
            }

            out += " ";

            if (hasFinal) // final is treated as an attribute, but it's really a keyword
                out += "final ";

            out += IndentBlock(get_Bases(), out.size() - (out.rfind('\n')+1));
            out  = out.substr(0, out.size()-1); // remove '\n'
            out += get_SizeComment();

            // data-members, methods, nested decls, etc.
            for (const clang::Decl* decl : cxxRecordDecl->decls())
            {
                if (decl->isImplicit())
                    continue;

                if (decl->getKind() == clang::Decl::Kind::AccessSpec)
                    out += SerializeDecl(contextItems, decl); // "public:", for instance, does not get indented
                else
                    out += IndentBlock(SerializeDecl(contextItems, decl), 3, "   ");

        //        if (const auto* enumDecl = clang::dyn_cast<clang::EnumDecl>(decl))
        //        {   // a locally defined enum
        //            out += ConstructEnumDefinition(enumDecl) + ";";
        //            continue;
        //        }

        //        if (const auto* nestedTemplate = clang::dyn_cast<clang::ClassTemplateDecl>(decl))
        //        {
        //            const clang::CXXRecordDecl* templated = nestedTemplate->getTemplatedDecl();
        //            if (!templated->isCompleteDefinition())
        //            {
        //                const clang::TemplateParameterList* params = nestedTemplate->getTemplateParameters();
        //                out += "   " + ConstructTemplateParameterList(params);
        //                out += templated->getKindName().str() + " ";
        //                out += nestedTemplate->getNameAsString() + ";\n";
        //                continue;
        //            }

        //            out += IndentBlock(ConstructRecordSignature(templated), 3, "   ");
        //            continue;
        //        }

        //        if (auto* friendDecl = dyn_cast<FriendDecl>(decl))
        //        {
        //            if (auto* namedFriendDecl = friendDecl->getFriendDecl())
        //            {
        //                if (auto* funcDecl = dyn_cast<FunctionDecl>(namedFriendDecl))
        //                {
        //                    out += IndentBlock(ConstructFunctionSignature(funcDecl, false), 3, "   ");
        //                    continue;
        //                }
        //                if (auto* funcTemplateDecl = dyn_cast<FunctionTemplateDecl>(namedFriendDecl))
        //                {
        //                    out += IndentBlock(ConstructFunctionSignature(funcTemplateDecl->getTemplatedDecl(), false), 3, "   ");
        //                    continue;
        //                }
        //                if (auto* classTemplateDecl = dyn_cast<ClassTemplateDecl>(namedFriendDecl))
        //                {
        //                    const auto* templated = classTemplateDecl->getTemplatedDecl();
        //                    if (!templated->isCompleteDefinition())
        //                    {
        //                        const clang::TemplateParameterList* params = classTemplateDecl->getTemplateParameters();
        //                        out += "   " + ConstructTemplateParameterList(params) + "friend ";
        //                        out += templated->getKindName().str() + " ";
        //                        out += classTemplateDecl->getQualifiedNameAsString() + ";\n";
        //                        continue;
        //                    }

        //                    out += IndentBlock(ConstructRecordSignature(templated), 3, "   ");
        //                    continue;
        //                }
        //            }
        //            if (auto* friendTypeSourceInfo = friendDecl->getFriendType())
        //            {
        //                out += "   friend " + friendTypeSourceInfo->getType().getAsString(printPolicy) + ";\n";
        //                continue;
        //            }
        //        }

        //        if (auto* typedefDecl = dyn_cast<TypedefDecl>(decl))
        //        {
        //            out += IndentBlock(ConstructTypedefSignature(typedefDecl), out.size() - (out.rfind('\n') + 1) + 3); // length of last line up to current spot (+3 for indenting)
        //            continue;
        //        }
        //        if (auto* typeAliasDecl = dyn_cast<TypeAliasDecl>(decl))
        //        {
        //            std::string      aliasName = typeAliasDecl->getNameAsString();
        //            QualType        underlying = typeAliasDecl->getUnderlyingType();
        //            if (const auto* recordType = underlying->getAs<RecordType>())
        //            {
        //                if (recordType->getDecl()->isInAnonymousNamespace())
        //                {
        //                    out += "   using " + aliasName + " = ";
        //                    out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), 12 + aliasName.size());
        //                    continue;
        //                }
        //            }
        //            out += "   using " + aliasName + " = " + underlying.getAsString(printPolicy) + ";\n";
        //            continue;
        //        }
        //        if (const auto* tatDecl = dyn_cast<TypeAliasTemplateDecl>(decl))
        //        {
        //            out += IndentBlock(ConstructTemplateAliasSignature(tatDecl), out.size() - (out.rfind('\n') + 1)); // length of last line up to current spot
        //            continue;
        //        }
        //            
        //        { // unhandled
        //            out += "unhandled clang::Decl ";
        //            out += decl->getDeclKindName();
        //            out += " ";
        //            if (const auto* named = clang::dyn_cast<clang::NamedDecl>(decl))
        //                out += named->getNameAsString();
        //            out += "\n";
        //        }
            }

            out += "};\n";

            return out;
        }
    };
}