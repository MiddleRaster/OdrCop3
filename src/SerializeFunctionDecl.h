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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class FunctionDeclSerializer
    {
        const ContextItems& contextItems;
        const FunctionDecl* funcDecl;
    public:
        class CXXMethodDeclSerializer
        {
            const ContextItems& contextItems;
            const CXXMethodDecl* methodDecl;
        public:
            CXXMethodDeclSerializer(const ContextItems& contextItems, const CXXMethodDecl* method) : contextItems(contextItems), methodDecl(method) {}

            std::string get_Const() const
            {
                if (methodDecl)
                if (methodDecl->isConst())
                    return "const ";
                return "";
            }
            std::string get_Volatile() const
            {
                if (methodDecl)
                if (methodDecl->isVolatile())
                    return "volatile ";
                return "";
            }
            std::string get_RefQualifier() const
            {
                if (methodDecl)
                {
                    switch (methodDecl->getRefQualifier())
                    {
                    default:
                    case RQ_None:   break;
                    case RQ_LValue: return "& ";
                    case RQ_RValue: return "&& ";
                    }
                }
                return "";
            }

            std::string get_Override() const
            {
                if (methodDecl)
                if (methodDecl->hasAttr<OverrideAttr>())
                    return "override ";
                return "";
            }
            std::string get_Final() const
            {
                if (methodDecl)
                if (methodDecl->hasAttr<FinalAttr>())
                    return "final ";
                return "";
            }
            std::string get_PureVirtual() const
            {
                if (methodDecl)
                if (methodDecl->isPureVirtual())
                    return "=0 ";
                return "";
            }
        };

        FunctionDeclSerializer(const ContextItems& contextItems, const FunctionDecl* funcDecl) : contextItems(contextItems), funcDecl(funcDecl) {}

        std::string get_TemplateHeader() const
        {
            if (const FunctionTemplateDecl* ftd = funcDecl->getDescribedFunctionTemplate())
            {
                std::string                 templatePrefix;
                llvm::raw_string_ostream os(templatePrefix);
                ftd->getTemplateParameters()->print(os, contextItems.context, contextItems.printPolicy);
                os.flush();
                return templatePrefix;
            }

            if (const auto* info = funcDecl->getTemplateSpecializationInfo())
            {
                switch (info->getTemplateSpecializationKind())
                {
                case clang::TSK_ExplicitInstantiationDeclaration:
                case clang::TSK_ExplicitInstantiationDefinition:
                    return "template ";

                case clang::TSK_ExplicitSpecialization:
                    return "template<> ";

                default:
                    break;
                }
            }

            return "";
        }

        std::string get_ReturnType() const
        {
            const auto* parentRecord = clang::dyn_cast<clang::CXXRecordDecl>(funcDecl->getParent());
            bool   isTemplateContext = funcDecl->getDescribedFunctionTemplate() != nullptr || (parentRecord && parentRecord->getDescribedClassTemplate() != nullptr);
            if (isTemplateContext)
                return funcDecl->getReturnType().getAsString(contextItems.printPolicy) + " ";
            //else if (const Type* ty = funcDecl->getReturnType().getTypePtr(); ty->isEnumeralType())
            //    fqn += ConstructEnumName(ty->getAs<EnumType>()->getDecl(), nullptr);

            if (funcDecl->getReturnType()->getAsCXXRecordDecl() &&
                funcDecl->getReturnType()->getAsCXXRecordDecl()->isLambda())
                return "auto "; // in case of returning a lambda

            //IndirectionCvStripper ics(funcDecl->getReturnType().getCanonicalType());
            //const QualType qualType = ics.GetBaseType();
            //const auto* recordType = dyn_cast<clang::RecordType>(qualType.getTypePtr());
            //if (recordType && recordType->getDecl()->isInAnonymousNamespace())
            //{
            //    fqn += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())),
            //        fqn.size() + ics.ConstructPrefix().size(),
            //        ics.ConstructPrefix());
            //    fqn = fqn.substr(0, fqn.size() - 2); // remove "; "
            //    fqn += ics.ConstructPointersAndReferences();
            //}
            //else
            //    fqn += qualType.getAsString(contextItems.printPolicy);

            return funcDecl->getReturnType().getAsString(contextItems.printPolicy) + " "; // for now
        }

        std::string get_ConstEval()       const { return funcDecl->isConsteval()                           ? "consteval " : ""; }
        std::string get_Constexpr()       const { return funcDecl->isConstexpr()                           ? "constexpr " : ""; }
        std::string get_InlineSpecified() const { return funcDecl->isInlineSpecified()                     ? "inline "    : ""; }
        std::string get_Virtual()         const { return funcDecl->isVirtualAsWritten()                    ? "virtual "   : ""; }
        std::string get_Extern()          const { return funcDecl->getStorageClass() == SC_Extern          ? "extern "    : ""; }
        std::string get_Register()        const { return funcDecl->getStorageClass() == SC_Register        ? "extern "    : ""; }
        std::string get_Static()          const { return funcDecl->isStatic()                              ? "static "    : ""; }
        std::string get_Friend()          const { return funcDecl->getFriendObjectKind() != Decl::FOK_None ? "friend "    : ""; }
        std::string get_Defaulted()       const { return funcDecl->isDefaulted()                           ? "=default "  : ""; }
        std::string get_Deleted()         const { return funcDecl->isDeleted()                             ? "=delete "   : ""; }
        std::string get_Export()          const { return funcDecl->isInExportDeclContext()                 ? "export "    : ""; }

        std::string get_ExceptionSpecifier() const
        {
            const auto* proto = funcDecl->getType()->getAs<FunctionProtoType>();
            if (proto != nullptr)
            {
                switch (proto->getExceptionSpecType())
                {
                case EST_BasicNoexcept:
                    if (funcDecl->getExceptionSpecSourceRange().isValid()) // only if the user actually wrote this (i.e., not "inferred" by the compiler)
                        return "noexcept ";
                    break;

                case EST_DependentNoexcept:
                {
                    std::string exprStr;
                    llvm::raw_string_ostream os(exprStr);
                    proto->getNoexceptExpr()->printPretty(os, nullptr, contextItems.printPolicy);
                    os.flush();
                    return "noexcept(" + exprStr + ") ";
                }
                case EST_Dynamic:
                {
                    std::string result = "throw(";
                    bool        first = true;
                    for (QualType t : proto->exceptions())
                    {
                        if (!first)
                            result += ", ";
                        result += t.getAsString(contextItems.printPolicy);
                        first = false;
                    }
                    return result + ") ";
                }
                case EST_NoexceptTrue:  return "noexcept(true) ";
                case EST_NoexceptFalse: return "noexcept(false) ";
                case EST_DynamicNone:   return "throw() ";
                case EST_MSAny:         return "throw(...) ";
                case EST_None:
                default:
                    break;
                }
            }
            return "";
        }

        std::string get_Explicit() const
        {
            if (const auto* ctor = dyn_cast<CXXConstructorDecl>(funcDecl))
                if (ctor->isExplicit())
                    return "explicit ";
            if (const auto* conv = dyn_cast<CXXConversionDecl>(funcDecl))
                if (conv->isExplicit())
                    return "explicit ";
            return "";
        }
        std::string get_LeadingAttributes() const
        {
            std::string out;
            SourceLocation nameEnd = funcDecl->getNameInfo().getEndLoc();
            for (const Attr * attr : funcDecl->attrs())
            {
                if (attr->getLocation() > nameEnd)
                    continue; // this is a trailing attribute: int f() [[attr]];
                out += SerializeAttr(contextItems, attr);
            }
            return out;
        }
        std::string get_TrailingAttributes() const
        {
            std::string out;
            SourceLocation nameEnd = funcDecl->getNameInfo().getEndLoc();
            for (const Attr * attr : funcDecl->attrs())
            {
                if (attr->getLocation() < nameEnd)
                    continue; // this is a leading attribute: [[attr]] int f();
                out += SerializeAttr(contextItems, attr);
            }
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
            switch (funcDecl->getType()->castAs<FunctionType>()->getCallConv())
            {
            case CC_X86StdCall:    return "__stdcall ";
            case CC_X86FastCall:   return "__fastcall ";
            case CC_X86ThisCall:   return "__thiscall ";
            case CC_X86VectorCall: return "__vectorcall ";
            case CC_Win64:         return "__ms_abi ";
            case CC_C: if (!(funcDecl->isExternC() || funcDecl->isMSVCRTEntryPoint())) 
                                   return "__cdecl "; // else fall through
            default:               return "";
            }
        }
        std::string get_FunctionName() const
        {
            if (const auto* conv = llvm::dyn_cast<clang::CXXConversionDecl>(funcDecl))
            {
                std::string typeName;
                llvm::raw_string_ostream os(typeName);
                conv->getConversionType().print(os, contextItems.printPolicy);
                return "operator " + typeName;
            }
            return funcDecl->getNameAsString();

            //{
            //    const FunctionDecl* funcDecl2 = funcDecl->getDescribedFunctionTemplate()
            //        ? funcDecl->getDescribedFunctionTemplate()->getTemplatedDecl()
            //        : funcDecl;
            //    if (const auto* conv = llvm::dyn_cast<clang::CXXConversionDecl>(funcDecl2))
            //    {
            //        std::string typeName;
            //        llvm::raw_string_ostream typeOs(typeName);
            //        conv->getConversionType().print(typeOs, contextItems.printPolicy);
            //        //if (wantFullyQualifiedMethodName)
            //        //{
            //        //    std::string className;
            //        //    llvm::raw_string_ostream classOs(className);
            //        //    conv->getParent()->printQualifiedName(classOs, printPolicy);
            //        //    fqn += className + "::operator " + typeName;
            //        //}
            //        //else
            //        fqn += "operator " + typeName;
            //    }
            //    //else if (wantFullyQualifiedMethodName)
            //    //{
            //    //    std::string              out;
            //    //    llvm::raw_string_ostream os(out);
            //    //    funcDecl2->printQualifiedName(os, printPolicy);
            //    //    os.flush();
            //    //    fqn += out;
            //    //}
            //    else
            //        fqn += funcDecl2->getNameAsString();

            //    //    if (const auto* args = funcDecl->getTemplateSpecializationArgs())
            //    //    {
            //    //        fqn += "<";
            //    //        bool first = true;
            //    //        for (const TemplateArgument& arg : args->asArray())
            //    //        {
            //    //            if (!first)
            //    //                fqn += ", ";
            //    //            first = false;

            //    //            bool handled = false;
            //    //            if (arg.getKind() == TemplateArgument::Type)
            //    //            {
            //    //                QualType qt = arg.getAsType();
            //    //                if (const auto* rd = qt.getTypePtr()->getAsCXXRecordDecl())
            //    //                {
            //    //                    if (rd->isInAnonymousNamespace() && rd->getIdentifier() != nullptr)
            //    //                    {
            //    //                        fqn += IndentBlock(ConstructRecordSignature(rd), fqn.size());
            //    //                        fqn = fqn.substr(0, fqn.size() - 2); // strip last ";\n"
            //    //                        handled = true;
            //    //                    }
            //    //                }
            //    //            }
            //    //            if (!handled)
            //    //            {
            //    //                llvm::raw_string_ostream os2(fqn);
            //    //                arg.print(printPolicy, os2, true);
            //    //                os2.flush();
            //    //            }
            //    //        }
            //    //        fqn += ">";
            //    //    }
            //}
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

            // post-process body

            // collapse to "{}" if there's no real content
            bool allWhitespace = true;
            for (char c : body) {
                if ((c == '{') ||
                    (c == '}'))
                    continue;
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    allWhitespace = false;
                    break;
                }
            }
            if (allWhitespace)
                body = "{}";

            // strip "this->"
            const std::string target = "this->";
            size_t pos = 0;
            while ((pos = body.find(target, pos)) != std::string::npos)
                body.erase(pos, target.length());

            // if it's a one-liner, remove all "\n   "
            int  semicolons = 0;
            for (char c : body) {
                if (c == ';')
                    ++semicolons;
            }
            if (semicolons < 2) {
                size_t pos = 0;
                while ((pos = body.find("\n", pos)) != std::string::npos)
                    body.replace(pos, 1, " ");

                pos = 0;
                while ((pos = body.find("  ", pos)) != std::string::npos)
                    body.replace(pos, 2, " ");

                while (body.ends_with(' '))
                    body = body.substr(0, body.size() - 1); // strip off last ' '

                body += "\n";
            }
            if (body.ends_with('\n'))
                body = body.substr(0, body.size()-1); // strip off last '\n'
            return body;
        }

        std::string Serialize() const
        {
            CXXMethodDeclSerializer method(contextItems, dyn_cast<CXXMethodDecl>(funcDecl));

            std::string fqn;

            fqn += get_TemplateHeader();
            fqn += get_LeadingAttributes();
            fqn += get_Friend();
            fqn += get_Register();
            fqn += get_Export();
            fqn += get_Static();
            fqn += get_Extern();
            fqn += get_Virtual();
            fqn += get_Explicit();
            fqn += get_InlineSpecified();
            fqn += get_Constexpr();
            fqn += get_ConstEval();
            fqn += get_ReturnType();
            fqn += get_CallingConvention();
            fqn += get_FunctionName();

            fqn += '(';
            for (const ParmVarDecl* param : funcDecl->parameters())
                fqn += SerializeDecl(contextItems, param) + ", ";              // TODO:  include parameter attributes
            if (fqn.substr(fqn.size()-2) == ", ")   // if there are args
                fqn = fqn.substr(0, fqn.size()-2);  // strip off last ", "
            fqn += ") ";

            fqn += get_TrailingRequiresClause();

            fqn += method.get_Const();
            fqn += method.get_Volatile();
            fqn += method.get_RefQualifier();
            fqn += get_ExceptionSpecifier();
            fqn += get_TrailingAttributes();
        //  fqn += method.get_Override(); // these two are actually attributes, which are handled on the line above
        //  fqn += method.get_Final();    // these two are actually attributes, which are handled on the line above
            fqn += method.get_PureVirtual();
            fqn += get_Defaulted();
            fqn += get_Deleted();
            fqn += get_ConstructorInitializers(); // if it's a ctor and if it has any initializers

            if (!(funcDecl->hasBody() && funcDecl->getBody()) || contextItems.doNotWantFunctionBody) // either there is no body, or we don't want to serialize the body
            { // no body:  end prototype with ';'
                if (fqn.ends_with(' '))
                    fqn = fqn.substr(0, fqn.size()-1);
                fqn += ";";
            } else
                fqn += get_Body();

            return fqn + "\n";
        }
    };
}