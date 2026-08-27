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

#include "magic_enum.h"
#include "SerializationUtils.h"
#include "SerializeCXXConversionDecl.h"
#include "SerializeCXXConstructorDecl.h"
#include "SerializeCXXDestructorDecl.h"
#include "SerializeFunctionDecl.h"
#include "SerializeFunctionTemplateDecl.h"
#include "SerializeCXXRecordDecl.h"
#include "SerializeEnumDecl.h"
#include "SerializeFieldDecl.h"
#include "SerializeVarDecl.h"
#include "SerializeParamVarDecl.h"
#include "SerializeAccessSpecDecl.h"
#include "SerializeTypedefDecl.h"
#include "SerializeTypeAliasDecl.h"
#include "SerializeTypeAliasTemplateDecl.h"
#include "SerializeClassTemplateDecl.h"
#include "SerializeClassTemplateSpecializationDecl.h"
#include "SerializeClassTemplatePartialSpecializationDecl.h"
#include "SerializeVarTemplateSpecializationDecl.h"
#include "SerializeVarTemplatePartialSpecializationDecl.h"
#include "SerializeFriendDecl.h"
#include "SerializeConceptDecl.h"

namespace OdrCop3
{
    namespace Serialize
    {
        class Can
        {
            class RecursionGuard
            {
                std::unordered_set<const clang::Decl*>& decls;
                const clang::Decl* decl;
                bool inserted;
            public:
                RecursionGuard(std::unordered_set<const clang::Decl*>& decls, const clang::Decl* decl)
                    : decls(decls)
                    , decl(decl)
                    , inserted(decls.insert(decl).second)
                {}
               ~RecursionGuard()
                {
                    if (inserted)
                        decls.erase(decl);
                }
                bool IsRecursing() const
                {
                    return !inserted;
                }
            };
            const ContextItems& contextItems;
            std::unordered_set<const clang::Decl*>& decls;

            bool IsUnnamedUnionClassOrStruct(const clang::Decl* decl) const
            {
                if (const auto* record = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
                    return record->getName().empty();
                return false;
            }
            bool IsUnnamedEnum(const clang::Decl* decl) const
            {
                if (const auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl))
                    return enumDecl->getName().empty();
                return false;
            }
            bool IsTemplateMethod(const clang::Decl* decl) const
            {
                if (llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
                    return true;
                return false;
            }
            bool IsTemplateClass(const clang::Decl* decl) const
            {
                if (llvm::dyn_cast<clang::ClassTemplateDecl>(decl))
                    return true;
                if (llvm::dyn_cast<clang::ClassTemplatePartialSpecializationDecl>(decl))
                    return true;
                return false;
            }
            bool IsVarInline(const clang::Decl* decl) const
            {
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    return varDecl->isInlineSpecified();
                return false;
            }
            bool IsVarOfUnnamedType(const clang::Decl* decl) const
            {
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    return !Can::PrintAnyOf(varDecl->getType());
                return false;
            }
            bool IsVarLambda(const clang::Decl* decl) const
            {
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    if (const auto* recordType = varDecl->getType().getNonReferenceType()->getAs<clang::RecordType>())
                        if (const auto* cxxRecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(recordType->getDecl()))
                            if (true == cxxRecordDecl->isLambda())
                                return true;
                return false;
            }
            bool IsVarTemplate(const clang::Decl* decl) const
            {
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    if (const auto* varTemplateDecl = varDecl->getDescribedVarTemplate())
                        return true;

                // Decl::print() prints these wrong
                if (dyn_cast<VarTemplateSpecializationDecl>(decl))
                    return true;
                if (dyn_cast<VarTemplatePartialSpecializationDecl>(decl))
                    return true;

                return false;
            }
            bool IsVarOutOfLine(const clang::Decl* decl) const
            {
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    return varDecl->isOutOfLine();
                return false;
            }
            bool IsFriendTemplateOrFunction(const clang::Decl* decl) const
            {
                if (const auto* friendDecl = llvm::dyn_cast<clang::FriendDecl>(decl))
                {
                    if (const auto* namedDecl = friendDecl->getFriendDecl())
                    {
                        if (nullptr != llvm::dyn_cast<clang::FunctionTemplateDecl>(namedDecl))
                            return true; // DeclPrinter::print() prints "friend" out front which is wrong.
                        if (nullptr != llvm::dyn_cast<clang::ClassTemplateDecl>(namedDecl))
                            return true; // DeclPrinter::print() prints "friend" out front which is wrong.
                        if (nullptr != llvm::dyn_cast<clang::FunctionDecl>(namedDecl))
                            return true; // DeclPrinter::print() prints a stray ";\n"
                    }
                }
                return false;
            }
            bool IsFieldAttributed(const clang::Decl* decl) const
            {
                if (const auto* fieldDecl = llvm::dyn_cast<clang::FieldDecl>(decl))
                    return !fieldDecl->attrs().empty();
                return false;
            }

            template <typename Type> bool PrintType(clang::QualType qualType) const
            {
                if (const auto* type = qualType->getAs<Type>())
                    return Can::Print(type->getDecl());
                return true;
            }
            bool PrintAnyOf(clang::QualType qualType) const
            {
                if (false == Can::PrintType<clang::TypedefType>(qualType))
                    return false;
                if (false == Can::PrintType<clang::RecordType >(qualType))
                    return false;
                if (false == Can::PrintType<clang::EnumType   >(qualType))
                    return false;
                
                if (qualType->isArrayType())
                    if (const auto* arrayType = qualType->getAsArrayTypeUnsafe())
                        if (false == Can::PrintAnyOf(arrayType->getElementType()))
                            return false;

                if (const auto* pointerType = qualType->getAs<clang::PointerType>())
                    if (false == Can::PrintAnyOf(pointerType->getPointeeType()))
                        return false;

                if (const auto* parenType = qualType->getAs<clang::ParenType>())
                    if (false == Can::PrintAnyOf(parenType->getInnerType()))
                        return false;

                if (const auto* functionProtoType = qualType->getAs<clang::FunctionProtoType>())
                {
                    if (false == Can::PrintAnyOf(functionProtoType->getReturnType().getNonReferenceType()))
                        return false;

                    for (clang::QualType paramType : functionProtoType->param_types())
                        if (false == Can::PrintAnyOf(paramType))
                            return false;
                }

                return true;
            }

            bool PrintReturnTypeAndArgs(const clang::Decl* decl) const
            {
                if (const FunctionDecl* functionDecl = dyn_cast<FunctionDecl>(decl))
                {   // return type, then args
                    if (false == Can::PrintAnyOf(functionDecl->getReturnType().getNonReferenceType()))
                        return false;
                    for (const ParmVarDecl* parmVarDecl : functionDecl->parameters())
                        if (false == Can::PrintAnyOf(parmVarDecl->getOriginalType()))
                            return false;
                }
                return true;
            }

        public:
            Can(const ContextItems& contextItems, std::unordered_set<const clang::Decl*>& decls) : contextItems(contextItems), decls(decls) {}
            bool Print(const clang::Decl* decl) const
            {
                RecursionGuard recursionGuard(decls, decl);
                if (recursionGuard.IsRecursing())
                    return true;

                if (contextItems.needsFriend == true)
                    return false;

                if (decl->getKind() == clang::Decl::Kind::AccessSpec)
                    return false; // AccessSpecDecl::print() prints nothing

                if (const auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl))
                    if (enumDecl->isScoped())
                        if (enumDecl->getIntegerTypeSourceInfo() == nullptr)
                            return false; // EnumDecl::print() adds implicit underlying type even if not in source (makes a false negative)

                if (NeedsManualSerialization(contextItems, decl) == true)
                    return false; // needs (anonymous namespace) type's definition inlined

                // recursively check declarations nested inside CXXRecordDecl*s (n levels deep)
                if (const auto* cxxRecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
                    for (const clang::Decl* childDecl : cxxRecordDecl->decls())
                        if (!childDecl->isImplicit())
                            if (false == Can::Print(childDecl))
                                return false;
                // ditto class template
                if (const auto* classTemplateDecl = llvm::dyn_cast<clang::ClassTemplateDecl>(decl))
                    if (false == Can::Print(classTemplateDecl->getTemplatedDecl()))
                        return false;
                if (false == Can::PrintReturnTypeAndArgs(decl)) // check function's return type and args
                    return false;

                // after recursion, check 1 level deep
                if (const auto* typeAliasDecl = llvm::dyn_cast<clang::TypeAliasDecl>(decl)) if (false == Can::PrintAnyOf(typeAliasDecl->getUnderlyingType()))             return false;
                if (const auto*   typedefDecl = llvm::dyn_cast<clang::  TypedefDecl>(decl)) if (false == Can::PrintAnyOf(  typedefDecl->getUnderlyingType()))             return false;
                if (const auto*   parmVarDecl = llvm::dyn_cast<clang::  ParmVarDecl>(decl)) if (false == Can::PrintAnyOf(  parmVarDecl->getOriginalType()))               return false;
                if (const auto*     fieldDecl = llvm::dyn_cast<clang::    FieldDecl>(decl)) if (false == Can::PrintAnyOf(    fieldDecl->getType()))                       return false;
                if (const auto*       varDecl = llvm::dyn_cast<clang::      VarDecl>(decl)) if (false == Can::PrintAnyOf(      varDecl->getType().getNonReferenceType())) return false;

                // after recursion (now top-level)
                if (true == IsUnnamedUnionClassOrStruct(decl))
                    return false;
                if (true == IsUnnamedEnum(decl))
                    return false;
                if (true == IsVarInline(decl))
                    return false;
                if (true == IsVarOfUnnamedType(decl))
                    return false;
                if (true == IsVarLambda(decl))
                    return false;
                if (true == IsVarTemplate(decl))
                    return false;
                if (true == IsFriendTemplateOrFunction(decl))
                    return false;
                if (true == IsVarOutOfLine(decl))
                    return false;
                if (true == IsFieldAttributed(decl))
                    return false;

                return true;
            }
        };

        class Needs
        {
        private:
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
                    return NestedNameSpecifierContainsAliasedName(declaratorDecl->getQualifier());
                if (const auto* tagDecl = llvm::dyn_cast<clang::TagDecl>(decl))
                    return NestedNameSpecifierContainsAliasedName(tagDecl->getQualifier());
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

                // Pointer types
                if (const auto* ptrType = qt->getAs<clang::PointerType>())
                    if (true == TypeContainsAliasedName(ptrType->getPointeeType()))
                        return true;

                // Reference types
                if (const auto* refType = qt->getAs<clang::ReferenceType>())
                    if (true == TypeContainsAliasedName(refType->getPointeeType()))
                        return true;

                // Array types: must go via Type*; getAs<ArrayType>() is forbidden.
                if (const clang::Type* rawType = qt.getTypePtr())
                    if (const auto* arrayType = llvm::dyn_cast<clang::ArrayType>(rawType))
                        if (true == TypeContainsAliasedName(arrayType->getElementType()))
                            return true;

                // Template specialization sugar: check template arguments for alias use
                if (const auto* tmplSpec = qt->getAs<clang::TemplateSpecializationType>())
                    for (const clang::TemplateArgument& arg : tmplSpec->template_arguments())
                        if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                            if (true == TypeContainsAliasedName(arg.getAsType()))
                                return true;

                return false;
            }
            static bool ExprContainsAliasedName(const clang::Expr* expr)
            {   // Walks an expression subtree. Needed because a namespace alias can appear inside an
                // initializer's Expr nodes (e.g. sizeof(Alias::Foo), a DeclRefExpr's own qualifier, or a
                // MemberExpr's own qualifier) with no path back through any Decl's type.
                if (!expr)
                    return false;

                if (const auto* declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(expr))
                    if (true == NestedNameSpecifierContainsAliasedName(declRefExpr->getQualifier()))
                        return true;

                if (const auto* memberExpr = llvm::dyn_cast<clang::MemberExpr>(expr))
                    if (true == NestedNameSpecifierContainsAliasedName(memberExpr->getQualifier()))
                        return true;

                if (const auto* traitExpr = llvm::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(expr))
                    if (traitExpr->isArgumentType())
                        if (true == TypeContainsAliasedName(traitExpr->getArgumentTypeInfo()->getType()))
                            return true;

                // Generic fallthrough: recurse into every child statement/expression so we don't
                // have to special-case every Expr subclass (CallExpr, CXXConstructExpr, etc.).
                for (const clang::Stmt* child : expr->children())
                    if (const auto* childExpr = llvm::dyn_cast_or_null<clang::Expr>(child))
                        if (true == ExprContainsAliasedName(childExpr))
                            return true;

                return false;
            }

        public:
            static bool OriginalNamespace(const clang::Decl* decl)
            {
                // Decl-side qualifiers
                if (true == QualifierContainsAliasedName(decl))
                    return true;

                // Type-side qualifiers (ValueDecls)
                if (const auto* valueDecl = llvm::dyn_cast<clang::ValueDecl>(decl))
                    if (true == TypeContainsAliasedName(valueDecl->getType()))
                        return true;

                // Initializer-side qualifiers (VarDecls) — e.g. sizeof(Alias::Foo) in the init.
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    if (true == ExprContainsAliasedName(varDecl->getInit()))
                        return true;

                // Recursively inspect child decls (fields, nested types, etc.)
                if (const auto* declContext = llvm::dyn_cast<clang::DeclContext>(decl))
                    for (const clang::Decl* child : declContext->decls())
                        if (!child->isImplicit())
                            if (true == Needs::OriginalNamespace(child))
                                return true;

                // class templates
                if (const auto* classTemplateDecl = llvm::dyn_cast<clang::ClassTemplateDecl>(decl))
                    if (const auto* cxxRecord = classTemplateDecl->getTemplatedDecl())
                        if (true == Needs::OriginalNamespace(cxxRecord))
                            return true;

                // functions and function templates
                const clang::FunctionDecl* functionDecl = nullptr;
                if (const auto* functionTemplateDecl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
                    functionDecl = functionTemplateDecl->getTemplatedDecl();
                else
                    functionDecl = llvm::dyn_cast<clang::FunctionDecl>(decl);
                if (functionDecl) {
                    for (const clang::ParmVarDecl* parm : functionDecl->parameters())
                        if (!parm->isImplicit())
                            if (true == Needs::OriginalNamespace(parm))
                                return true;
                    if (true == TypeContainsAliasedName(functionDecl->getReturnType()))
                        return true;
                }
                if (const auto* conversionDecl = llvm::dyn_cast<clang::CXXConversionDecl>(decl))
                    if (true == TypeContainsAliasedName(conversionDecl->getConversionType()))
                        return true;

                // Base classes on CXXRecordDecl
                if (const auto* cxxRecord = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
                    if (cxxRecord->isThisDeclarationADefinition())
                        for (const clang::CXXBaseSpecifier& base : cxxRecord->bases())
                            if (true == TypeContainsAliasedName(base.getType()))
                                return true;

                return false;
            }
        };

        struct Semicolon
        {
            static std::string IfNeeded(const std::string& str, const clang::Decl* decl)
            {
                switch (decl->getKind())
                {
                case clang::Decl::FunctionTemplate: return cast<clang::FunctionTemplateDecl>(decl)->getTemplatedDecl()->hasBody() ? "" : ";";
                case clang::Decl::CXXConstructor:
                case clang::Decl::CXXConversion:
                case clang::Decl::CXXDestructor:
                case clang::Decl::CXXMethod:
                case clang::Decl::Function:
                    if (cast<clang::FunctionDecl>(decl)->hasBody())
                        return "";
                    if (str.ends_with("}"))
                        return "";
                    if (str.ends_with("}\n"))
                        return "";
                    return ";\n";
                default:
                    break;
                }
                return ";\n"; // everything else needs this
            }
        };

        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr, bool resolveNamespaceAliases>
        static inline std::string CallSerializer(const ContextItems& contextItems, const clang::Decl* decl)
        {
            switch(decl->getKind())
            {
            case clang::Decl::Kind::VarTemplateSpecialization:          if (const VarTemplateSpecializationDecl*           vtsd = dyn_cast<         VarTemplateSpecializationDecl>(decl)) return          VarTemplateSpecializationDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,               vtsd).Serialize(); break;
            case clang::Decl::Kind::VarTemplatePartialSpecialization:   if (const VarTemplatePartialSpecializationDecl*   vtpsd = dyn_cast<  VarTemplatePartialSpecializationDecl>(decl)) return   VarTemplatePartialSpecializationDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,              vtpsd).Serialize(); break;
            case clang::Decl::Kind::ClassTemplatePartialSpecialization: if (const ClassTemplatePartialSpecializationDecl* ctpsd = dyn_cast<ClassTemplatePartialSpecializationDecl>(decl)) return ClassTemplatePartialSpecializationDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,              ctpsd).Serialize(); break;
            case clang::Decl::Kind::ClassTemplateSpecialization:        if (const ClassTemplateSpecializationDecl*         ctsd = dyn_cast<       ClassTemplateSpecializationDecl>(decl)) return        ClassTemplateSpecializationDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,               ctsd).Serialize(); break;
            case clang::Decl::Kind::ClassTemplate:                      if (const ClassTemplateDecl*                        ctd = dyn_cast<                     ClassTemplateDecl>(decl)) return                      ClassTemplateDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,                ctd).Serialize(); break;
            case clang::Decl::Kind::FunctionTemplate:                   if (const FunctionTemplateDecl*                     ftd = dyn_cast<                  FunctionTemplateDecl>(decl)) return                   FunctionTemplateDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,                ftd).Serialize(); break;
            case clang::Decl::Kind::CXXMethod: // is a subclass of FunctionDecl
            case clang::Decl::Kind::Function:                           if (const FunctionDecl*                    functionDecl = dyn_cast<                          FunctionDecl>(decl)) return                           FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,       functionDecl).Serialize(); break;
            case clang::Decl::Kind::CXXConversion:                      if (const CXXConversionDecl*          cxxConversionDecl = dyn_cast<                     CXXConversionDecl>(decl)) return                      CXXConversionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,  cxxConversionDecl).Serialize(); break;
            case clang::Decl::Kind::CXXConstructor:                     if (const CXXConstructorDecl*        cxxConstructorDecl = dyn_cast<                    CXXConstructorDecl>(decl)) return                     CXXConstructorDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems, cxxConstructorDecl).Serialize(); break;
            case clang::Decl::Kind::CXXDestructor:                      if (const CXXDestructorDecl*          cxxDestructorDecl = dyn_cast<                     CXXDestructorDecl>(decl)) return                      CXXDestructorDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,  cxxDestructorDecl).Serialize(); break;
            case clang::Decl::Kind::ParmVar:                            if (const ParmVarDecl *                             pvd = dyn_cast<                           ParmVarDecl>(decl)) return                            ParmVarDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,                pvd).Serialize(); break;
            case clang::Decl::Kind::CXXRecord:                          if (const CXXRecordDecl *                 cxxRecordDecl = dyn_cast<                         CXXRecordDecl>(decl)) return                          CXXRecordDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,      cxxRecordDecl).Serialize(); break;
            case clang::Decl::Kind::Field:                              if (const FieldDecl *                         fieldDecl = dyn_cast<                             FieldDecl>(decl)) return                              FieldDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,          fieldDecl).Serialize(); break;
            case clang::Decl::Kind::AccessSpec:                         if (const AccessSpecDecl *                   accessDecl = dyn_cast<                        AccessSpecDecl>(decl)) return                         AccessSpecDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,         accessDecl).Serialize(); break;
            case clang::Decl::Kind::Var:                                if (const VarDecl *                             varDecl = dyn_cast<                               VarDecl>(decl)) return                                VarDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,            varDecl).Serialize(); break;
            case clang::Decl::Kind::Enum:                               if (const EnumDecl*                            enumDecl = dyn_cast<                              EnumDecl>(decl)) return                               EnumDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,           enumDecl).Serialize(); break;
            case clang::Decl::Kind::Typedef:                            if (const TypedefDecl *                     typedefDecl = dyn_cast<                           TypedefDecl>(decl)) return                            TypedefDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,        typedefDecl).Serialize(); break;
            case clang::Decl::Kind::TypeAlias:                          if (const TypeAliasDecl *                           tad = dyn_cast<                         TypeAliasDecl>(decl)) return                          TypeAliasDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,                tad).Serialize(); break;
            case clang::Decl::Kind::TypeAliasTemplate:                  if (const TypeAliasTemplateDecl*                   tatd = dyn_cast<                 TypeAliasTemplateDecl>(decl)) return                  TypeAliasTemplateDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,               tatd).Serialize(); break;
            case clang::Decl::Kind::Friend:                             if (const FriendDecl *                       friendDecl = dyn_cast<                            FriendDecl>(decl)) return                             FriendDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,         friendDecl).Serialize(); break;
            case clang::Decl::Kind::Concept:                            if (const ConceptDecl *                     conceptDecl = dyn_cast<                           ConceptDecl>(decl)) return                            ConceptDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,        conceptDecl).Serialize(); break;
            default: break;
            }
            // when this is released, comment out the next two lines, so that it won't throw but will print something
            decl->dump();
            throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));

            std::string str;
            llvm::raw_string_ostream os(str);
            decl->print(os, contextItems.printPolicy);
            os.flush();
            return str + Semicolon::IfNeeded(str, decl);
        }

        template<auto SerializeType, auto SerializeExpr>
        inline std::string Decls(const ContextItems& contextItems, const clang::Decl* decl)
        {
            class RecursionPreventor
            {
                std::unordered_set<const clang::Decl*>& recursingDecls;
                const bool recursing;
                const clang::Decl* decl;
            public:
                RecursionPreventor(std::unordered_set<const clang::Decl*>& recursingDecls, const clang::Decl* decl)
                    : recursingDecls(recursingDecls)
                    , recursing(recursingDecls.find(decl) != recursingDecls.end())
                    , decl(decl)
                {
                    recursingDecls.insert(decl);
                }
                bool IsRecursing() const { return recursing; }
               ~RecursionPreventor()
                {
                    recursingDecls.erase(decl);
                }
            } recursionPreventor(contextItems.recursingDecls, decl);
            if (recursionPreventor.IsRecursing() == true)
            {
                clang::QualType qualType;
                if      (const auto* valueDecl = llvm::dyn_cast<clang::ValueDecl>(decl)) qualType = valueDecl->getType();
                else if (const auto*  typeDecl = llvm::dyn_cast<clang::TypeDecl >(decl)) qualType =  typeDecl->getTypeForDecl()->getCanonicalTypeInternal();
                else return std::string{};

                while (qualType->isPointerType() || qualType->isReferenceType())
                    qualType = qualType->getPointeeType();

                return qualType.getAsString();
            }

            bool resolveNamespaceAliases = Needs::OriginalNamespace(decl);
            std::unordered_set<const clang::Decl*> decls;
            if ((Can(contextItems, decls).Print(decl) == false) || (resolveNamespaceAliases == true))
            {
                if (resolveNamespaceAliases)
                    return CallSerializer<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr, true >(contextItems, decl);
                else
                    return CallSerializer<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr, false>(contextItems, decl);
            }

            std::string str;
            llvm::raw_string_ostream os(str);
            clang::PrintingPolicy policy(contextItems.printPolicy);
            decl->print(os, policy);
            os.flush();
            return str + Semicolon::IfNeeded(str, decl);
        }
    }
}