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

#include "TemplateSerializationUtils.h"
#include "SerializeAttrs.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr, bool resolveNamespaceAliases=false> class FunctionDeclSerializer
    {
        const ContextItems& contextItems;
        const FunctionDecl* funcDecl;

        class CXXMethodDeclSerializer
        {
            const ContextItems& contextItems;
            const CXXMethodDecl* methodDecl;
        public:
            CXXMethodDeclSerializer(const ContextItems& contextItems, const CXXMethodDecl* method) : contextItems(contextItems), methodDecl(method) {}
            std::string get_RefQualifier() const
            {
                if     (methodDecl)
                switch (methodDecl->getRefQualifier())
                {
                default:
                case RQ_None:   break;
                case RQ_LValue: return "& ";
                case RQ_RValue: return "&& ";
                }
                return "";
            }
            std::string get_Const      () const { return methodDecl && methodDecl->isConst              () ? "const "    : ""; }
            std::string get_Volatile   () const { return methodDecl && methodDecl->isVolatile           () ? "volatile " : ""; }
            std::string get_Override   () const { return methodDecl && methodDecl->hasAttr<OverrideAttr>() ? "override " : ""; }
            std::string get_Final      () const { return methodDecl && methodDecl->hasAttr<   FinalAttr>() ? "final "    : ""; }
            std::string get_PureVirtual() const { return methodDecl && methodDecl->isPureVirtual        () ? "= 0 "      : ""; }
        };

        std::string get_TemplateSpecializationHeader() const
        {
            if (const auto* info = funcDecl->getTemplateSpecializationInfo())
            {
                switch (info->getTemplateSpecializationKind())
                {
                case clang::TSK_ExplicitInstantiationDeclaration:
                case clang::TSK_ExplicitInstantiationDefinition : return "template ";
                case clang::TSK_ExplicitSpecialization          : return "template<> ";
                default: break;
                }
            }
            return "";
        }

        struct IsReturnType
        {
            static bool EventuallyArray(QualType qt)
            {
                if (const auto* pointerType = qt->getAs<PointerType>())
                    return EventuallyArray(pointerType->getPointeeType());

                if (const auto* referenceType = qt->getAs<ReferenceType>())
                    return EventuallyArray(referenceType->getPointeeType());

                if (qt->isArrayType())
                    return true;

                return false;
            }
        };

        std::string get_ReturnType()      const
        {
            if (IsReturnType::EventuallyArray(funcDecl->getReturnType()))
            {
                // if a function returning a reference to an array, the syntax is tricky:
                // int (&ReturningReferenceTo1DArrayOfInts(int,double) noexcept)[3] { return blah; }
                // Everything from the "int" to the closing ) before the "[3]" goes into aux.

                std::string aux = SerializeFromCallingConventionToTrailingReturn([&]() { return ""; }, [&]() { return get_FunctionName(); });
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, TrimRightIf(aux, " "));
                std::string out = SerializeType(ci2, funcDecl->getReturnType());
                return TrimRightIf(out, " ");
            }
            return TrimRightIf(SerializeType(contextItems, resolveNamespaceAliases ? funcDecl->getReturnType().getCanonicalType() : funcDecl->getReturnType()), " ");
        }
        std::string get_ConstEval()       const { return funcDecl->isConsteval()                           ? "consteval "    : ""; }
        std::string get_InlineSpecified() const { return funcDecl->isInlineSpecified()                     ? "inline "       : ""; }
        std::string get_Virtual()         const { return funcDecl->isVirtualAsWritten()                    ? "virtual "      : ""; }
        std::string get_Extern()          const { return funcDecl->getStorageClass() == SC_Extern          ? "extern "       : ""; }
        std::string get_Register()        const { return funcDecl->getStorageClass() == SC_Register        ? "extern "       : ""; }
        std::string get_Static()          const { return funcDecl->isStatic()                              ? "static "       : ""; }
        std::string get_Friend()          const { return contextItems.needsFriend                          ? "friend "       : ""; }
        std::string get_Defaulted()       const { return funcDecl->isDefaulted()                           ? "= default "    : ""; }
        std::string get_Deleted()         const { return funcDecl->isDeleted()                             ? "= delete "     : ""; }
        std::string get_Variadic()        const { return funcDecl->isVariadic()                            ? "..."           : ""; }
        std::string get_Constexpr()       const
        {
                                                 // this matches Decl::Print()'s behavior
            if (funcDecl->isConstexprSpecified() && !funcDecl->isExplicitlyDefaulted())
                return "constexpr ";
            return "";
        }
        std::string get_ExceptionSpecifier() const
        {
            return GetExceptionSpecifier<SerializeDecl, SerializeType, SerializeExpr>(contextItems, funcDecl->getType()->getAs<FunctionProtoType>(), funcDecl);
        }
        std::string get_Explicit() const
        {
            bool isExplicit = false;
            ExplicitSpecifier explicitSpecifier;

            if (const auto * ctor = dyn_cast<CXXConstructorDecl>(funcDecl)) {
                isExplicit        = ctor->isExplicit();
                explicitSpecifier = ctor->getExplicitSpecifier();
            }
            if (const auto* conv = dyn_cast<CXXConversionDecl>(funcDecl)) {
                isExplicit = conv->isExplicit();
                explicitSpecifier = conv->getExplicitSpecifier();
            }

            if (explicitSpecifier.isSpecified())              // user wrote explicit(...)
                if (auto* expr = explicitSpecifier.getExpr()) // the original condition expression (may be null for plain explicit)
                    return "explicit(" + IndentBlock(SerializeExpr(contextItems, expr), 9) + ") ";

            if (isExplicit)
                return "explicit ";
            return "";
        }
        std::string get_LeadingAttributes() const
        {
            std::string out;
            SourceLocation nameEnd = funcDecl->getNameInfo().getEndLoc();
            for (const Attr * attr : funcDecl->attrs())
                if (attr->getLocation() <= nameEnd)
                    out += SerializeAttr(contextItems, attr);
            return out;
        }
        std::string get_TrailingAttributes() const
        {
            std::string out;
            SourceLocation nameEnd = funcDecl->getNameInfo().getEndLoc();
            for (const Attr * attr : funcDecl->attrs())
                if (attr->getLocation() >= nameEnd)
                    out += SerializeAttr(contextItems, attr);
            return out;
        }
        std::string get_TrailingRequiresClause() const
        {
            const  AssociatedConstraint  & associatedContraint = funcDecl->getTrailingRequiresClause();
            if (const Expr* requiresExpr = associatedContraint.ConstraintExpr)
            {
                std::string out = "requires ";
                llvm::raw_string_ostream os(out);
                requiresExpr->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();
                return out + " ";
            }
            return "";
        }
        std::string get_CallingConvention() const
        {
            return ""; // Clang uses attributes for these, but (according to Copilot) doesn't use one for __cdecl. I'll have to test this.
        }
        std::string get_ConstructorInitializers() const
        {
            std::string out;
            if (const auto* ctor = clang::dyn_cast<clang::CXXConstructorDecl>(funcDecl))
            {
                if (ctor->init_begin() != ctor->init_end())
                {
                    out += ": ";
                    bool first = true;
                    for (const clang::CXXCtorInitializer* init : ctor->inits())
                    {
                        if (!init->isWritten())
                            continue;  // skip implicit initializers

                        if (first)
                            first = false;
                        else
                            out += ", ";

                        if (init->isBaseInitializer())
                            out += clang::QualType(init->getBaseClass(), 0).getAsString(contextItems.printPolicy);
                        else if (init->isMemberInitializer())
                            out += init->getMember()->getNameAsString();
                        else if (init->isIndirectMemberInitializer())
                            out += init->getIndirectMember()->getNameAsString();
                        // else if (init->isDelegatingInitializer())
                        //     ;

                        std::string argStr;
                        llvm::raw_string_ostream os(argStr);
                        init->getInit()->printPretty(os, nullptr, contextItems.printPolicy);
                        os.flush();
                        out += "(" + argStr + ")";
                    }
                    out += " ";
                }
            }
            return out;
        }
        std::string get_Body() const
        {
            std::string body;
            llvm::raw_string_ostream os(body);
            funcDecl->getBody()->printPretty(os, nullptr, contextItems.printPolicy);
            os.flush();
            return body;
        }
        bool hasTrailingReturn() const
        {
            if (const auto* fpt = funcDecl->getType()->getAs<clang::FunctionProtoType>())
                return fpt->hasTrailingReturn();
            return false; // K&R, pre-C99, etc.
        }


        std::string SerializeFromCallingConventionToTrailingReturn(auto returnType, auto functionName) const
        {
            CXXMethodDeclSerializer method(contextItems, dyn_cast<CXXMethodDecl>(funcDecl));
            std::string fqn;
            fqn += get_CallingConvention();
            fqn += IndentBlock(functionName(), LengthOfLastLine(fqn));
            fqn += '(';
            for (const ParmVarDecl* param : funcDecl->parameters())
            {
                fqn += TrimRightIf(IndentBlock(SerializeDecl(contextItems, param), LengthOfLastLine(fqn)), ";");
                fqn += ", ";
            }
            fqn += get_Variadic();
            fqn  = TrimRightIf(fqn, ", ");
            fqn += ") ";
            fqn += method.get_Const();
            fqn += method.get_Volatile();
            fqn += method.get_RefQualifier();
            fqn += IndentBlock(get_ExceptionSpecifier(), LengthOfLastLine(fqn));
            fqn += get_TrailingAttributes();
            if (true == hasTrailingReturn())
            {
                fqn += "-> ";
                fqn += IndentBlock(returnType(), LengthOfLastLine(fqn));
                fqn += " ";
            }
            return fqn;
        }

        std::string SerializePastFriend(auto returnType, auto functionName) const
        {
            CXXMethodDeclSerializer method(contextItems, dyn_cast<CXXMethodDecl>(funcDecl));

            std::string fqn;
            fqn += get_Register();
            fqn += get_Static();
            fqn += get_Extern();
            fqn += get_Virtual();
            fqn += IndentBlock(get_Explicit(), LengthOfLastLine(fqn));
            fqn += get_InlineSpecified();
            fqn += get_Constexpr();
            fqn += get_ConstEval();

            if (true == hasTrailingReturn())
                fqn += "auto "; // has trailing-return syntax
            else
                fqn += IndentBlock(returnType(), LengthOfLastLine(fqn));
            if (fqn.empty() == false)
            if (fqn.substr(fqn.size()-1) != "*") // e.g., "void *" gets no space
            if (fqn.substr(fqn.size()-1) != "&") // e.g., ditto &
            if (fqn.substr(fqn.size()-1) != " ") // certainly don't want two spaces in a row
                fqn += " ";                      // e.g., "int" does

            if (false == IsReturnType::EventuallyArray(funcDecl->getReturnType())) // if not that returning-reference-to-array syntax
                fqn += IndentBlock(SerializeFromCallingConventionToTrailingReturn(returnType, functionName), LengthOfLastLine(fqn));

            fqn += get_TrailingRequiresClause();
            fqn += method.get_PureVirtual();
            fqn += get_Defaulted();
            fqn += get_Deleted();
            fqn += get_ConstructorInitializers(); // if it's a ctor and if it has any initializers

            if (!(funcDecl->hasBody() && funcDecl->getBody()) || !contextItems.wantFunctionBody) // either there is no body, or we don't want to serialize the body
                fqn = TrimRightIf(fqn, " ") + ";"; // no body:  end prototype with ';'
            else
                fqn += TrimRightIf(get_Body(), "\n");
            return fqn + "\n";
        }
        std::string SerializeUpToFriend() const
        {
            std::string fqn;
            fqn += get_TemplateSpecializationHeader();
            fqn += get_LeadingAttributes();
            fqn += get_Friend();
            return fqn;
        }
    public:
        FunctionDeclSerializer(const ContextItems& contextItems, const FunctionDecl* funcDecl) : contextItems(contextItems), funcDecl(funcDecl) {}
    protected:
        std::string get_FunctionName() const
        {
            std::string name = funcDecl->getNameAsString();
            if (auto *  args = funcDecl->getTemplateSpecializationArgs())
            {   // explicit specialization
                std::string out = "<";

                for (unsigned i=0; i<args->size(); ++i)
                {
                    if (i > 0)
                        out += ", ";

                    std::string str;
                    {
                        llvm::raw_string_ostream os(str);
                        args->get(i).print(contextItems.printPolicy, os, false);
                        os.flush();

                        // strip off <> from template packs, if any
                        if (str.starts_with("<")) str = str.substr(1);
                        str = TrimRightIf(str, ">");
                    }
                    out += str;
                }
                out += ">";
                name += out;
            }
            return name;
        }

        static size_t GetIndentation(const std::string& prefix, const std::string& block)
        {   // similar to function of same name in SerializationUtils

            auto pos = block.find("\n");
            if (pos != std::string::npos)
                if (block.size() > pos + 1 + 5) // 1 to get past "\n" and 5 for 5 spaces
                    if (0 == block.compare(pos + 1, 5, "     "))
                        return prefix.size();

            // if returning a type defined in an anonymous namespace the check above doesn't work.
            if (block.find("(anonymous namespace)::") != std::string::npos)
                return prefix.size();
            // N.B.: NOTE: TODO: REVIEW:  the 2 lines above are NOT in SerializationUtils. Figure out why they're different.

            return 0;
        }

        std::string Serialize(auto returnType, auto functionName) const
        {
            std::string fqn;
            fqn += SerializeUpToFriend();

            // turn off ContextItems::needsFriend so that Can::Print() can return true
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, contextItems.aux);
            ci2.needsFriend      = false;
            ci2.wantFunctionBody = contextItems.wantFunctionBody;
            std::string block    = FunctionDeclSerializer(ci2, funcDecl).SerializePastFriend(returnType, functionName);

            fqn += IndentBlock(block, GetIndentation(fqn, block));
            return fqn + "\n";
        }

    public:
        std::string Serialize() const
        {
            return Serialize([&]() { return get_ReturnType  (); },
                             [&]() { return get_FunctionName(); });
        }
    };
}