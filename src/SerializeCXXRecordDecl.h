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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class CXXRecordDeclSerializer
    {
        const ContextItems & contextItems;
        const CXXRecordDecl* cxxRecordDecl;
    public:
        CXXRecordDeclSerializer(const ContextItems& contextItems, const CXXRecordDecl* cxxRecordDecl) : contextItems(contextItems), cxxRecordDecl(cxxRecordDecl) {}

        std::string get_Kind()        const { return cxxRecordDecl->getKindName().str() + " "; }
        std::string get_Name()        const { return MakeUnnamedAndAnonymousConsistent(cxxRecordDecl->getQualifiedNameAsString()); }
        std::string get_SizeComment() const { return cxxRecordDecl->isCompleteDefinition() && !cxxRecordDecl->isDependentType() ? " // sizeof=" + std::to_string(contextItems.context.getASTRecordLayout(cxxRecordDecl).getSize().getQuantity()) + "\n" : "\n"; }
        std::string get_Friend()      const { return contextItems.needsFriend ? "friend " : ""; }

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
                out  = TrimRightIf(out, ";");
            }
            if (firstBase == false)
                out += " ";

            out += "{";
            return out;
        }

        std::string Serialize() const
        {
#ifdef KEEP_THIS_INTERESTING_IDEA
            {   // the idea is to user the AST's printing facility if we can, if there are no "anonymous namespace" types.
                // Unfortuantely, to see through typedefs/using aliases means we must set PrintAsCanonical,
                // and that messes up template parameter types ("type-parameter-0-0", rather than "T").

                clang::PrintingPolicy policy = contextItems.printPolicy;
                policy.FullyQualifiedName = true;
                policy.PrintAsCanonical   = true;

                std::string str;
                llvm::raw_string_ostream os(str);
                cxxRecordDecl->print(os, policy);
                os.flush();

                if (str.find("(anonymous namespace)") == std::string::npos)
                {   // do 3 minor fixups:  
                    
                    // add sizeof comment
                    std::size_t pos = str.find('\n');
                    if (pos != std::string::npos)
                        str.insert(pos, TrimRightIf(get_SizeComment(), "\n"));

                    // change 4 spaces to 3.
                    std::string from = "\n    ";
                    std::string to   = "\n   ";
                    while ((pos = str.find(from, pos)) != std::string::npos)
                    {
                        str.replace(pos, from.size(), to);
                        pos += to.size();
                    }

                    // append ";\n"
                    str += ";\n";

                    return str;
                }
            }
#endif

            std::string out;

            if (cxxRecordDecl->isInjectedClassName())
                return out;
            if (cxxRecordDecl->isLambda())
                return out;

            out += get_Friend();
            out += get_Kind(); // struct/class/union keyword

            bool hasFinal = false; // final is treated as an attribute, but it's really a keyword
            out += get_Attributes(&hasFinal);
            out += get_Name() + contextItems.aux; // for any <args>. NOTE: no " " until after template stuff

            if (!cxxRecordDecl->isThisDeclarationADefinition())
                return out + ";\n"; // if it's a declaration, go no farther

            out += " ";
            if (hasFinal) // final is treated as an attribute, but it's really a keyword
                out += "final ";

            out += IndentBlock(get_Bases(), out.size() - (out.rfind('\n')+1));
            out += get_SizeComment();

            // data-members, methods, nested decls, etc.
            for (const clang::Decl* decl : cxxRecordDecl->decls())
            {
                if (decl->isImplicit())
                    continue;

                if (decl->getKind() == clang::Decl::Kind::AccessSpec)
                    out += SerializeDecl(contextItems, decl); // "public:", for instance, does not get indented
                else
                    out += IndentBlock(SerializeDecl(contextItems, decl), 3, "   ") + "\n";
            }
            out += "};\n";
            return out;
        }
    };
}