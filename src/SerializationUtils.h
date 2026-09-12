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
#include <algorithm>

namespace OdrCop3
{
    struct ContextItems
    {
        ASTContext& context;
        const PrintingPolicy& printPolicy;
        const std::string& TU;
        std::unordered_set<const Decl*>& recursingDecls;
        std::string aux;
        bool wantFunctionBody       = true;
        bool needsFriend            = false;
        bool suppressTemplatePrefix = false;
        ContextItems(ASTContext* context, const PrintingPolicy& policy, const std::string& TU, std::unordered_set<const Decl*>& recursingDecls, const std::string& aux="")
            : context       (*context)
            , printPolicy   (policy)
            , TU            (TU)
            , recursingDecls(recursingDecls)
            , aux           (aux)
        {}

        ContextItems withWantFunctionBody(bool value) const
        {
            auto result = *this;
            result.wantFunctionBody = value;
            return result;
        }
        ContextItems withNeedsFriend(bool value = true) const
        {
            auto result = *this;
            result.needsFriend = value;
            return result;
        }
        ContextItems withSuppressTemplatePrefix(bool value = true) const
        {
            auto result = *this;
            result.suppressTemplatePrefix = value;
            return result;
        }
        ContextItems withAux(std::string value) const
        {
            auto result = *this;
            result.aux = std::move(value);
            return result;
        }
    };

	struct UnhandledException : public std::exception
	{
		const std::string message;
        UnhandledException(const std::string& msg) : message(msg) {}
		const char* what() const noexcept override { return message.c_str(); }
	};

    inline std::string TrimRightIf(std::string out, const std::string& what)
    {
        if (out.ends_with(what))
            out = out.substr(0, out.size()-what.size());
        return out;
    }
    inline size_t LengthOfLastLine(const std::string& out) { return out.size() - (out.rfind('\n')+1); }
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
        return TrimRightIf(out, "\n");
    }

    inline std::string MakeUnnamedAndAnonymousConsistent(std::string input)
    {
        auto Replace = [](std::string& str, std::string_view bad, std::string_view good) { if (auto pos = str.find(bad); pos != std::string::npos) str.replace(pos, bad.size(), good); };
        Replace(input, "(unnamed at",          "(anonymous type at");
        Replace(input, "(unnamed enum at",     "(anonymous type at");
        Replace(input, "(unnamed union at",    "(anonymous type at");
        Replace(input, "(anonymous struct at", "(anonymous type at");
        Replace(input, "(anonymous class at",  "(anonymous type at");
        Replace(input, "(anonymous union at",  "(anonymous type at");
        return input;
    }


    template<typename PrintLambda> inline bool NeedsManualSerialization(const ContextItems& contextItems, PrintLambda print)
    {
        clang::PrintingPolicy policy = contextItems.printPolicy;
        policy.FullyQualifiedName = true;
        policy.PrintAsCanonical   = true;

        std::string str;
        llvm::raw_string_ostream os(str);
        print(os, policy);
        os.flush();

        if (str.find("(anonymous namespace)") != std::string::npos)
            return true;
        return false;
    }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, QualType qt                                              ) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) {                     qt.print      (os,                       policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::TemplateParameterList* templateParameterList) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) { templateParameterList->print      (os, contextItems.context, policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::Expr                 *                  expr) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) {                  expr->printPretty(os, nullptr             , policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::Decl                 *                  decl)
    {
        if (decl->isInAnonymousNamespace())
            return true; // print() often doesn't print "(anonymous namespace)"
        return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) { decl->print(os, policy); });
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr>
    inline std::string GetExceptionSpecifier(const ContextItems& contextItems, const FunctionProtoType* functionProtoType, const FunctionDecl* functionDecl)
    {
        switch (functionProtoType->getExceptionSpecType())
        {
        case EST_Dynamic:
        {
            std::string result = "throw(";
            bool first = true;
            for (QualType t : functionProtoType->exceptions())
            {
                if (first)
                    first = false;
                else
                    result += ", ";
                result += t.getAsString(contextItems.printPolicy);
            }
            return result + ") ";
        }
        case EST_NoexceptTrue:      //return "noexcept(true) ";
        case EST_NoexceptFalse:     //return "noexcept(false) ";
        case EST_DependentNoexcept: return "noexcept(" + IndentBlock(SerializeExpr(contextItems, functionProtoType->getNoexceptExpr()), 9) + ") ";
        case EST_BasicNoexcept:     return "noexcept ";
        case EST_DynamicNone:       return "throw() ";
        case EST_MSAny:             return "throw(...) ";
        case EST_None:
        default:
            break;
        }
        return "";
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TemplateDeclBaseSerializer
    {
        const ContextItems& contextItems;

        static size_t GetIndentation(const std::string& prefix, const std::string& block)
        {
            // idea: examine block. 
            // If it is multi-line AND the second line starts with more than 4 spaces, then we know that some "(anonymous namespace)" type was serialized inline.
            // In that case, we need to indent by an extra prefix.size(); otherwise, we can just concatenate it or indent by 0.

            size_t indent = 0;
            auto pos = block.find("\n");
            if (pos != std::string::npos)
                if (block.size() > pos+1 + 5) // 1 to get past "\n" and 5 for 5 spaces
                    if (0 == block.compare(pos + 1, 5, "     "))
                        indent = prefix.size();

            return indent;
        }
    public:
        TemplateDeclBaseSerializer(const ContextItems& contextItems) : contextItems(contextItems) {}
        std::string Serialize(const clang::TemplateParameterList* templateParameterList, const clang::Decl* decl) const
        {
            std::string prefix = GetTemplateHeader<SerializeDecl, SerializeType, SerializeExpr>(contextItems, templateParameterList);
            std::string block  = SerializeDecl(contextItems.withSuppressTemplatePrefix(true), decl);
            return prefix + IndentBlock(block, GetIndentation(prefix, block)) + "\n";
        }
    };

    inline std::string SnugUpPointersAndReferences(const std::string& str)
    {
        if (!str.ends_with("*")) // e.g., "void *" gets no space
        if (!str.ends_with("&")) // e.g., ditto &
        if (!str.ends_with(" ")) // certainly don't want two spaces in a row
            return " ";          // e.g., "int" does
        return "";
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> inline std::string SerializeTemplateArgument(const ContextItems& contextItems, const TemplateArgument& arg, size_t indent)
    {
        switch (arg.getKind())
        {
        case clang::TemplateArgument::Type       : return TrimRightIf(IndentBlock(SerializeType(contextItems, arg.getAsType()), indent), ";");
        case clang::TemplateArgument::Expression : return             IndentBlock(SerializeExpr(contextItems, arg.getAsExpr()), indent);
        case clang::TemplateArgument::Declaration: return             IndentBlock(SerializeDecl(contextItems, arg.getAsDecl()), indent);
        default:
            break;
        }

        std::string argStr;
        llvm::raw_string_ostream os(argStr);
        arg.print(contextItems.printPolicy, os, true);
        os.flush();
        return argStr;
    }

    struct IsType
    {
        static bool EventuallyArrayOrFunctionPointer(QualType qt)
        {
            if (const auto*       pointerType = qt->getAs<      PointerType>()) return EventuallyArrayOrFunctionPointer(      pointerType->getPointeeType());
            if (const auto*     referenceType = qt->getAs<    ReferenceType>()) return EventuallyArrayOrFunctionPointer(    referenceType->getPointeeType());
            if (const auto* memberPointerType = qt->getAs<MemberPointerType>()) return EventuallyArrayOrFunctionPointer(memberPointerType->getPointeeType());

            return qt->isArrayType() || qt->isFunctionType();
        }
    };

    class Needs
    {
        struct OriginalNamespaceVisitor : public clang::RecursiveASTVisitor<OriginalNamespaceVisitor>
        {
            bool found = false;

            bool TraverseTypeConstraint(const clang::TypeConstraint* constraint)
            {
                if (constraint)
                    if (const clang::ConceptReference* conceptRef = constraint->getConceptReference())
                        if (NestedNameSpecifierContainsAliasedName(conceptRef->getNestedNameSpecifierLoc().getNestedNameSpecifier()))
                            found = true;

                if (!found)
                    return clang::RecursiveASTVisitor<OriginalNamespaceVisitor>::TraverseTypeConstraint(constraint);
                return !found;
            }
            bool TraverseFunctionTemplateDecl(clang::FunctionTemplateDecl* functionTemplateDecl)
            {
                llvm::SmallVector<clang::AssociatedConstraint, 4> constraints;
                functionTemplateDecl->getAssociatedConstraints(constraints);
                for (const clang::AssociatedConstraint& constraint : constraints)
                    if (constraint.ConstraintExpr != nullptr)
                        if (!TraverseStmt(const_cast<clang::Expr*>(constraint.ConstraintExpr)))
                            return false;
                if (!found)
                    return clang::RecursiveASTVisitor<OriginalNamespaceVisitor>::TraverseFunctionTemplateDecl(functionTemplateDecl);
                return !found;
            }
            bool TraverseConceptExprRequirement(clang::concepts::ExprRequirement* requirement)
            {
                if (!requirement->isExprSubstitutionFailure())
                    if (const clang::Expr* expr = requirement->getExpr()) {
                        if (TypeContainsAliasedName(expr->getType()))
                            found = true;
                        if (!found)
                            if (!TraverseStmt(const_cast<clang::Expr*>(expr)))
                                return false;
                    }
                if (!found && requirement->getReturnTypeRequirement().isTypeConstraint())
                    if (const clang::TypeConstraint* constraint = requirement->getReturnTypeRequirement().getTypeConstraint())
                        TraverseTypeConstraint(constraint);
                return !found;
            }
            bool TraverseConceptTypeRequirement(clang::concepts::TypeRequirement* requirement)
            {
                if (!requirement->isSubstitutionFailure())
                    if (const clang::TypeSourceInfo* typeSourceInfo = requirement->getType())
                        if (TypeContainsAliasedName(typeSourceInfo->getType()))
                            found = true;
                return !found;
            }
            bool TraverseConceptNestedRequirement(clang::concepts::NestedRequirement* requirement)
            {
                if (!requirement->hasInvalidConstraint())
                    TraverseStmt(const_cast<clang::Expr*>(requirement->getConstraintExpr()));
                return !found;
            }
            bool TraverseNestedNameSpecifier(clang::NestedNameSpecifier nns)
            {
                if (nns.getKind() == clang::NestedNameSpecifier::Kind::Namespace)
                    if (clang::isa<clang::NamespaceAliasDecl>(nns.getAsNamespaceAndPrefix().Namespace))
                        found = true;
                return RecursiveASTVisitor<OriginalNamespaceVisitor>::TraverseNestedNameSpecifier(nns);
            }

            bool VisitDecl(const clang::Decl* decl)
            {
                if (QualifierContainsAliasedName(decl))
                    found = true;
                if (const auto* valueDecl = llvm::dyn_cast<clang::ValueDecl>(decl))
                    if (TypeContainsAliasedName(valueDecl->getType()))
                        found = true;
                if (const auto* typedefNameDecl = llvm::dyn_cast<clang::TypedefNameDecl>(decl))
                    if (TypeContainsAliasedName(typedefNameDecl->getUnderlyingType()))
                        found = true;
                return !found;
            }
            bool VisitConceptReference(clang::ConceptReference* conceptRef)
            {
                if (NestedNameSpecifierContainsAliasedName(conceptRef->getNestedNameSpecifierLoc().getNestedNameSpecifier()))
                    found = true;
                return !found;
            }
            bool VisitDeclRefExpr(const clang::DeclRefExpr* expr)
            {
                if (NestedNameSpecifierContainsAliasedName(expr->getQualifier()))
                    found = true;
                return !found;
            }
            bool VisitMemberExpr(const clang::MemberExpr* expr)
            {
                if (NestedNameSpecifierContainsAliasedName(expr->getQualifier()))
                    found = true;
                return !found;
            }
            bool VisitConceptDecl(const clang::ConceptDecl* decl)
            {
                if (TemplateParametersContainAliasedName(decl->getTemplateParameters()))
                    found = true;
                return !found;
            }
            bool VisitConceptSpecializationExpr(const clang::ConceptSpecializationExpr* expr)
            {
                if (const clang::ConceptReference* conceptRef = expr->getConceptReference())
                    if (NestedNameSpecifierContainsAliasedName(conceptRef->getNestedNameSpecifierLoc().getNestedNameSpecifier()))
                        found = true;
                if (!found)
                    for (const clang::TemplateArgument& arg : expr->getTemplateArguments())
                        if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                            if (TypeContainsAliasedName(arg.getAsType()))
                                found = true;
                return !found;
            }
            bool VisitUnaryExprOrTypeTraitExpr(const clang::UnaryExprOrTypeTraitExpr* expr)
            {
                if (expr->isArgumentType())
                    if (TypeContainsAliasedName(expr->getArgumentTypeInfo()->getType()))
                        found = true;
                return !found;
            }
            bool VisitTypeTraitExpr(const clang::TypeTraitExpr* expr)
            {
                for (const clang::TypeSourceInfo* arg : expr->getArgs())
                    if (TypeContainsAliasedName(arg->getType()))
                        found = true;
                return !found;
            }
            bool VisitRequiresExpr(const clang::RequiresExpr* expr)
            {
                for (const clang::ParmVarDecl* parm : expr->getLocalParameters())
                    if (TypeContainsAliasedName(parm->getType()))
                        found = true;
                return !found;
            }
            bool VisitNonTypeTemplateParmDecl(const clang::NonTypeTemplateParmDecl* decl)
            {
                if (TypeContainsAliasedName(decl->getType()))
                    found = true;
                return !found;
            }
            bool VisitCXXConversionDecl(const clang::CXXConversionDecl* decl)
            {
                if (TypeContainsAliasedName(decl->getConversionType()))
                    found = true;
                return !found;
            }
            bool VisitCXXRecordDecl(const clang::CXXRecordDecl* decl)
            {
                if (NestedNameSpecifierContainsAliasedName(decl->getQualifier()))
                    found = true;
                if (decl->isThisDeclarationADefinition())
                    for (const clang::CXXBaseSpecifier& base : decl->bases())
                        if (TypeContainsAliasedName(base.getType()))
                            found = true;
                return !found;
            }
            bool VisitClassTemplateSpecializationDecl(const clang::ClassTemplateSpecializationDecl* decl)
            {
                for (const clang::TemplateArgument& arg : decl->getTemplateArgs().asArray())
                    if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                        if (TypeContainsAliasedName(arg.getAsType()))
                            found = true;
                return !found;
            }
            bool VisitFunctionDecl(const clang::FunctionDecl* decl)
            {
                if (TypeContainsAliasedName(decl->getReturnType()))
                    found = true;
                return !found;
            }

        private:
            static bool TemplateParametersContainAliasedName(const clang::TemplateParameterList* params)
            {
                if (params)
                    for (const clang::NamedDecl* param : *params)
                        if (const auto* ntp = llvm::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
                            if (TypeContainsAliasedName(ntp->getType()))
                                return true;
                return false;
            }
            static bool TemplateArgsContainAliasedName(const clang::CXXRecordDecl* cxxRecordDecl)
            {   // pulls template arguments directly off a ClassTemplateSpecializationDecl,
                // for cases where the TemplateSpecializationType sugar has already been stripped away.
                if (const auto* specDecl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl))
                    for (const clang::TemplateArgument& arg : specDecl->getTemplateArgs().asArray())
                        if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                            if (true == TypeContainsAliasedName(arg.getAsType()))
                                return true;
                return false;
            }
            static bool NestedNameSpecifierContainsAliasedName(clang::NestedNameSpecifier nestedNameSpecifier)
            {
                while (nestedNameSpecifier)
                {
                    switch (nestedNameSpecifier.getKind())
                    {
                    case clang::NestedNameSpecifier::Kind::Namespace:
                        if (llvm::isa<clang::NamespaceAliasDecl>(nestedNameSpecifier.getAsNamespaceAndPrefix().Namespace))
                            return true;
                        nestedNameSpecifier = nestedNameSpecifier.getAsNamespaceAndPrefix().Prefix;
                        break;
                    case clang::NestedNameSpecifier::Kind::Type:
                        // A Type-kind qualifier can itself be a template specialization carrying
                        // an aliased argument, so run it through the full TypePrintingType check.
                        if (const clang::Type* type = nestedNameSpecifier.getAsType())
                            return TypeContainsAliasedName(clang::QualType(type, 0));
                        return false;
                    default:
                        return false;
                    }
                }
                return false;
            }
            static bool QualifierContainsAliasedName(const clang::Decl* decl)
            {
                if (const auto* declaratorDecl = llvm::dyn_cast<clang::DeclaratorDecl>(decl))
                    if (NestedNameSpecifierContainsAliasedName(declaratorDecl->getQualifier()))
                        return true;
                if (const auto* tagDecl = llvm::dyn_cast<clang::TagDecl>(decl))
                    if (NestedNameSpecifierContainsAliasedName(tagDecl->getQualifier()))
                        return true;
                return false;
            }
            static bool TypeContainsAliasedName(clang::QualType qt)
            {
                if (const auto* recordType = qt->getAs<clang::RecordType>())
                {
                    if (true == NestedNameSpecifierContainsAliasedName(recordType->getQualifier()))
                        return true;
                    if (const auto* cxxRecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(recordType->getDecl()))
                        if (true == TemplateArgsContainAliasedName(cxxRecordDecl))
                            return true;
                }

                if (const auto* enumType = qt->getAs<clang::EnumType>())
                    if (true == NestedNameSpecifierContainsAliasedName(enumType->getQualifier()))
                        return true;

                if (const auto* typedefType = qt->getAs<clang::TypedefType>())
                    if (true == NestedNameSpecifierContainsAliasedName(typedefType->getQualifier()))
                        return true;

                if (const auto* ptrType = qt->getAs<clang::PointerType>())
                    if (true == TypeContainsAliasedName(ptrType->getPointeeType()))
                        return true;

                if (const auto* refType = qt->getAs<clang::ReferenceType>())
                    if (true == TypeContainsAliasedName(refType->getPointeeType()))
                        return true;

                if (const clang::Type* rawType = qt.getTypePtr())
                    if (const auto* arrayType = llvm::dyn_cast<clang::ArrayType>(rawType))
                        if (true == TypeContainsAliasedName(arrayType->getElementType()))
                            return true;

                if (const auto* tmplSpec = qt->getAs<clang::TemplateSpecializationType>())
                    for (const clang::TemplateArgument& arg : tmplSpec->template_arguments())
                        if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                            if (true == TypeContainsAliasedName(arg.getAsType()))
                                return true;

                return false;
            }
        };

    public:
        static bool OriginalNamespace(const clang::Decl* decl)
        {
            OriginalNamespaceVisitor visitor;
            visitor.TraverseDecl(const_cast<clang::Decl*>(decl));
            return visitor.found;
        }
        static bool OriginalNamespace(const clang::QualType qualType)
        {
            OriginalNamespaceVisitor visitor;
            visitor.TraverseType(qualType);
            return visitor.found;
        }
        static bool OriginalNamespace(const clang::Expr* expr)
        {
            OriginalNamespaceVisitor visitor;
            visitor.TraverseStmt(const_cast<clang::Expr*>(expr));
            return visitor.found;
        }
    };
}