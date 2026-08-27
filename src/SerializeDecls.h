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
        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr>
        struct Decl
        {
            static std::string SerializeFunctionTemplateDecl                  (const ContextItems& contextItems, const FunctionTemplateDecl            * functionTemplateDecl) { return FunctionTemplateDeclSerializer                  <SerializeDecl, SerializeType, SerializeExpr>(contextItems,  functionTemplateDecl).Serialize(); }
            static std::string SerializeFunctionDecl                          (const ContextItems& contextItems, const FunctionDecl                                * funcDecl) { return FunctionDeclSerializer                          <SerializeDecl, SerializeType, SerializeExpr>(contextItems,              funcDecl).Serialize(); }
            static std::string SerializeCXXConversionDecl                     (const ContextItems& contextItems, const CXXConversionDecl                  * cxxConversionDecl) { return CXXConversionDeclSerializer                     <SerializeDecl, SerializeType, SerializeExpr>(contextItems,     cxxConversionDecl).Serialize(); }
            static std::string SerializeCXXConstructorDecl                    (const ContextItems& contextItems, const CXXConstructorDecl                * cxxConstructorDecl) { return CXXConstructorDeclSerializer                    <SerializeDecl, SerializeType, SerializeExpr>(contextItems,    cxxConstructorDecl).Serialize(); }
            static std::string SerializeCXXDestructorDecl                     (const ContextItems& contextItems, const CXXDestructorDecl                  * cxxDestructorDecl) { return CXXDestructorDeclSerializer                     <SerializeDecl, SerializeType, SerializeExpr>(contextItems,     cxxDestructorDecl).Serialize(); }
            static std::string SerializeAccessSpecDecl                        (const ContextItems& contextItems, const AccessSpecDecl                            * accessDecl) { return AccessSpecDeclSerializer                        <SerializeDecl, SerializeType, SerializeExpr>(contextItems,            accessDecl).Serialize(); }
            static std::string SerializeTypedefDecl                           (const ContextItems& contextItems, const TypedefDecl                              * typedefDecl) { return TypedefDeclSerializer                           <SerializeDecl, SerializeType, SerializeExpr>(contextItems,           typedefDecl).Serialize(); }
            static std::string SerializeTypeAliasDecl                         (const ContextItems& contextItems, const TypeAliasDecl                          * typeAliasDecl) { return TypeAliasDeclSerializer                         <SerializeDecl, SerializeType, SerializeExpr>(contextItems,         typeAliasDecl).Serialize(); }
            static std::string SerializeTypeAliasTemplateDecl                 (const ContextItems& contextItems, const TypeAliasTemplateDecl          * typeAliasTemplateDecl) { return TypeAliasTemplateDeclSerializer                 <SerializeDecl, SerializeType, SerializeExpr>(contextItems, typeAliasTemplateDecl).Serialize(); }
            static std::string SerializeFieldDecl                             (const ContextItems& contextItems, const FieldDecl                                  * fieldDecl) { return FieldDeclSerializer                             <SerializeDecl, SerializeType, SerializeExpr>(contextItems,             fieldDecl).Serialize(); }
            static std::string SerializeVarDecl                               (const ContextItems& contextItems, const VarDecl                                      * varDecl) { return VarDeclSerializer                               <SerializeDecl, SerializeType, SerializeExpr>(contextItems,               varDecl).Serialize(); }
            static std::string SerializeEnumDecl                              (const ContextItems& contextItems, const EnumDecl                                    * enumDecl) { return EnumDeclSerializer                              <SerializeDecl, SerializeType, SerializeExpr>(contextItems,              enumDecl).Serialize(); }
            static std::string SerializeParmVarDecl                           (const ContextItems& contextItems, const ParmVarDecl                             *  parmVarDecl) { return ParmVarDeclSerializer                           <SerializeDecl, SerializeType, SerializeExpr>(contextItems,           parmVarDecl).Serialize(); }
            static std::string SerializeCXXRecordDecl                         (const ContextItems& contextItems, const CXXRecordDecl                          * cxxRecordDecl) { return CXXRecordDeclSerializer                         <SerializeDecl, SerializeType, SerializeExpr>(contextItems,         cxxRecordDecl).Serialize(); }
            static std::string SerializeClassTemplateDecl                     (const ContextItems& contextItems, const ClassTemplateDecl                                * ctd) { return ClassTemplateDeclSerializer                     <SerializeDecl, SerializeType, SerializeExpr>(contextItems,                   ctd).Serialize(); }
            static std::string SerializeClassTemplateSpecializationDecl       (const ContextItems& contextItems, const ClassTemplateSpecializationDecl                 * ctsd) { return ClassTemplateSpecializationDeclSerializer       <SerializeDecl, SerializeType, SerializeExpr>(contextItems,                  ctsd).Serialize(); }
            static std::string SerializeClassTemplatePartialSpecializationDecl(const ContextItems& contextItems, const ClassTemplatePartialSpecializationDecl         * ctpsd) { return ClassTemplatePartialSpecializationDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems,                 ctpsd).Serialize(); }
            static std::string SerializeVarTemplateSpecializationDecl         (const ContextItems& contextItems, const VarTemplateSpecializationDecl                   * vtsd) { return VarTemplateSpecializationDeclSerializer         <SerializeDecl, SerializeType, SerializeExpr>(contextItems,                  vtsd).Serialize(); }
            static std::string SerializeVarTemplatePartialSpecializationDecl  (const ContextItems& contextItems, const VarTemplatePartialSpecializationDecl           * vtpsd) { return VarTemplatePartialSpecializationDeclSerializer  <SerializeDecl, SerializeType, SerializeExpr>(contextItems,                 vtpsd).Serialize(); }
            static std::string SerializeFriendDecl                            (const ContextItems& contextItems, const FriendDecl                                * friendDecl) { return FriendDeclSerializer                            <SerializeDecl, SerializeType, SerializeExpr>(contextItems,            friendDecl).Serialize(); }
            static std::string SerializeConceptDecl                           (const ContextItems& contextItems, const ConceptDecl                              * conceptDecl) { return ConceptDeclSerializer                           <SerializeDecl, SerializeType, SerializeExpr>(contextItems,           conceptDecl).Serialize(); }
        };

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

            std::unordered_set<const clang::Decl*> decls;
            if (Can(contextItems, decls).Print(decl) == false)
            {
                using DeclSerializer = Serialize::Decl<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr>;
                switch(decl->getKind())
                {
                case clang::Decl::Kind::VarTemplateSpecialization:          if (const VarTemplateSpecializationDecl*           vtsd = dyn_cast<VarTemplateSpecializationDecl         >(decl)) return DeclSerializer::SerializeVarTemplateSpecializationDecl         (contextItems, vtsd);               break;
                case clang::Decl::Kind::VarTemplatePartialSpecialization:   if (const VarTemplatePartialSpecializationDecl*   vtpsd = dyn_cast<VarTemplatePartialSpecializationDecl  >(decl)) return DeclSerializer::SerializeVarTemplatePartialSpecializationDecl  (contextItems, vtpsd);              break;
                case clang::Decl::Kind::ClassTemplatePartialSpecialization: if (const ClassTemplatePartialSpecializationDecl* ctpsd = dyn_cast<ClassTemplatePartialSpecializationDecl>(decl)) return DeclSerializer::SerializeClassTemplatePartialSpecializationDecl(contextItems, ctpsd);              break;
                case clang::Decl::Kind::ClassTemplateSpecialization:        if (const ClassTemplateSpecializationDecl*         ctsd = dyn_cast<ClassTemplateSpecializationDecl       >(decl)) return DeclSerializer::SerializeClassTemplateSpecializationDecl       (contextItems, ctsd);               break;
                case clang::Decl::Kind::ClassTemplate:                      if (const ClassTemplateDecl*                        ctd = dyn_cast<ClassTemplateDecl                     >(decl)) return DeclSerializer::SerializeClassTemplateDecl                     (contextItems, ctd);                break;
                case clang::Decl::Kind::FunctionTemplate:                   if (const FunctionTemplateDecl *                    ftd = dyn_cast<FunctionTemplateDecl                  >(decl)) return DeclSerializer::SerializeFunctionTemplateDecl                  (contextItems, ftd);                break;
                case clang::Decl::Kind::CXXMethod: // is a subclass of FunctionDecl
                case clang::Decl::Kind::Function:                           if (const FunctionDecl*                    functionDecl = dyn_cast<FunctionDecl                          >(decl)) return DeclSerializer::SerializeFunctionDecl                          (contextItems, functionDecl);       break;
                case clang::Decl::Kind::CXXConversion:                      if (const CXXConversionDecl*          cxxConversionDecl = dyn_cast<CXXConversionDecl                     >(decl)) return DeclSerializer::SerializeCXXConversionDecl                     (contextItems, cxxConversionDecl);  break;
                case clang::Decl::Kind::CXXConstructor:                     if (const CXXConstructorDecl*        cxxConstructorDecl = dyn_cast<CXXConstructorDecl                    >(decl)) return DeclSerializer::SerializeCXXConstructorDecl                    (contextItems, cxxConstructorDecl); break;
                case clang::Decl::Kind::CXXDestructor:                      if (const CXXDestructorDecl*          cxxDestructorDecl = dyn_cast<CXXDestructorDecl                     >(decl)) return DeclSerializer::SerializeCXXDestructorDecl                     (contextItems, cxxDestructorDecl);  break;
                case clang::Decl::Kind::ParmVar:                            if (const ParmVarDecl *                             pvd = dyn_cast<ParmVarDecl                           >(decl)) return DeclSerializer::SerializeParmVarDecl                           (contextItems, pvd);                break;
                case clang::Decl::Kind::CXXRecord:                          if (const CXXRecordDecl *                           cxx = dyn_cast<CXXRecordDecl                         >(decl)) return DeclSerializer::SerializeCXXRecordDecl                         (contextItems, cxx);                break;
                case clang::Decl::Kind::Field:                              if (const FieldDecl *                         fieldDecl = dyn_cast<FieldDecl                             >(decl)) return DeclSerializer::SerializeFieldDecl                             (contextItems, fieldDecl);          break;
                case clang::Decl::Kind::AccessSpec:                         if (const AccessSpecDecl *                   accessDecl = dyn_cast<AccessSpecDecl                        >(decl)) return DeclSerializer::SerializeAccessSpecDecl                        (contextItems, accessDecl);         break;
                case clang::Decl::Kind::Var:                                if (const VarDecl *                             varDecl = dyn_cast<VarDecl                               >(decl)) return DeclSerializer::SerializeVarDecl                               (contextItems, varDecl);            break;
                case clang::Decl::Kind::Enum:                               if (const EnumDecl*                            enumDecl = dyn_cast<EnumDecl                              >(decl)) return DeclSerializer::SerializeEnumDecl                              (contextItems, enumDecl);           break;
                case clang::Decl::Kind::Typedef:                            if (const TypedefDecl *                     typedefDecl = dyn_cast<TypedefDecl                           >(decl)) return DeclSerializer::SerializeTypedefDecl                           (contextItems, typedefDecl);        break;
                case clang::Decl::Kind::TypeAlias:                          if (const TypeAliasDecl *                           tad = dyn_cast<TypeAliasDecl                         >(decl)) return DeclSerializer::SerializeTypeAliasDecl                         (contextItems, tad);                break;
                case clang::Decl::Kind::TypeAliasTemplate:                  if (const TypeAliasTemplateDecl*                   tatd = dyn_cast<TypeAliasTemplateDecl                 >(decl)) return DeclSerializer::SerializeTypeAliasTemplateDecl                 (contextItems, tatd);               break;
                case clang::Decl::Kind::Friend:                             if (const FriendDecl *                       friendDecl = dyn_cast<FriendDecl                            >(decl)) return DeclSerializer::SerializeFriendDecl                            (contextItems, friendDecl);         break;
                case clang::Decl::Kind::Concept:                            if (const ConceptDecl *                     conceptDecl = dyn_cast<ConceptDecl                           >(decl)) return DeclSerializer::SerializeConceptDecl                           (contextItems, conceptDecl);        break;
                default: break;
                }
                // when this is released, comment out the next two lines, so that it won't throw but will print something
                decl->dump();
                throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));
            }

            class Needs
            {
            private:
                static bool TemplateArgsPrintingType(const clang::CXXRecordDecl* cxxRecordDecl)
                {   // pulls template arguments directly off a ClassTemplateSpecializationDecl,
                    // for cases where the TemplateSpecializationType sugar has already been stripped away.
                    if (const auto* specDecl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(cxxRecordDecl))
                        for (const clang::TemplateArgument& arg : specDecl->getTemplateArgs().asArray())
                            if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                                if (true == TypePrintingType(arg.getAsType()))
                                    return true;
                    return false;
                }
                static bool ClassifyNestedNameSpecifier(clang::NestedNameSpecifier nestedNameSpecifier)
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
                                return TypePrintingType(clang::QualType(type, 0));
                            return false;
                        default:
                            return false;
                        }
                    }
                    return false;
                }
                static bool DeclQualifierPrintingType(const clang::Decl* decl)
                {
                    if (const auto* declaratorDecl = llvm::dyn_cast<clang::DeclaratorDecl>(decl))
                        return ClassifyNestedNameSpecifier(declaratorDecl->getQualifier());
                    if (const auto* tagDecl = llvm::dyn_cast<clang::TagDecl>(decl))
                        return ClassifyNestedNameSpecifier(tagDecl->getQualifier());
                    return false;
                }
                static bool TypePrintingType(clang::QualType qt)
                {
                    if (const auto* recordType = qt->getAs<clang::RecordType>())
                    {
                        if (true == ClassifyNestedNameSpecifier(recordType->getQualifier()))
                            return true;
                        // for template args living on the decl
                        if (const auto* cxxRecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(recordType->getDecl()))
                            if (true == TemplateArgsPrintingType(cxxRecordDecl))
                                return true;
                    }

                    if (const auto* enumType = qt->getAs<clang::EnumType>())
                        if (true == ClassifyNestedNameSpecifier(enumType->getQualifier()))
                            return true;

                    if (const auto* typedefType = qt->getAs<clang::TypedefType>())
                        if (true == ClassifyNestedNameSpecifier(typedefType->getQualifier()))
                            return true;

                    // Pointer types
                    if (const auto* ptrType = qt->getAs<clang::PointerType>())
                        if (true == TypePrintingType(ptrType->getPointeeType()))
                            return true;

                    // Reference types
                    if (const auto* refType = qt->getAs<clang::ReferenceType>())
                        if (true == TypePrintingType(refType->getPointeeType()))
                            return true;

                    // Array types: must go via Type*; getAs<ArrayType>() is forbidden.
                    if (const clang::Type* rawType = qt.getTypePtr())
                        if (const auto* arrayType = llvm::dyn_cast<clang::ArrayType>(rawType))
                            if (true == TypePrintingType(arrayType->getElementType()))
                                return true;

                    // Template specialization sugar: check template arguments for alias use
                    if (const auto* tmplSpec = qt->getAs<clang::TemplateSpecializationType>())
                        for (const clang::TemplateArgument& arg : tmplSpec->template_arguments())
                            if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                                if (true == TypePrintingType(arg.getAsType()))
                                    return true;

                    return false;
                }

            public:
                static bool OriginalNamespace(const clang::Decl* decl)
                {
                    // Decl-side qualifiers
                    if (true == DeclQualifierPrintingType(decl))
                        return true;

                    // Type-side qualifiers (ValueDecls)
                    if (const auto* valueDecl = llvm::dyn_cast<clang::ValueDecl>(decl))
                        if (true == TypePrintingType(valueDecl->getType()))
                            return true;

                    // Recursively inspect child decls (fields, nested types, etc.)
                    if (const auto* declContext = llvm::dyn_cast<clang::DeclContext>(decl))
                        for (const clang::Decl* child : declContext->decls())
                            if (!child->isImplicit())
                                if (true == Needs::OriginalNamespace(child))
                                    return true;
                    // the code above handle function parameters, but not return types
                    if (const auto* functionDecl = llvm::dyn_cast<clang::FunctionDecl>(decl))
                        if (true == TypePrintingType(functionDecl->getReturnType()))
                            return true;
                    if (const auto* conversionDecl = llvm::dyn_cast<clang::CXXConversionDecl>(decl))
                        if (true == TypePrintingType(conversionDecl->getConversionType()))
                            return true;

                    // Base classes on CXXRecordDecl
                    if (const auto* cxxRecord = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
                        for (const clang::CXXBaseSpecifier& base : cxxRecord->bases())
                            if (true == TypePrintingType(base.getType()))
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

            std::string str;
            llvm::raw_string_ostream os(str);
            clang::PrintingPolicy policy(contextItems.printPolicy);
            if (false == Needs::OriginalNamespace(decl))
                decl->print(os, policy);
            else
            {
                switch(decl->getKind())
                {
              //case clang::Decl::Kind::VarTemplateSpecialization:          if (const VarTemplateSpecializationDecl*           vtsd = dyn_cast<VarTemplateSpecializationDecl         >(decl)) return DeclSerializer::SerializeVarTemplateSpecializationDecl         (contextItems, vtsd);               break;
              //case clang::Decl::Kind::VarTemplatePartialSpecialization:   if (const VarTemplatePartialSpecializationDecl*   vtpsd = dyn_cast<VarTemplatePartialSpecializationDecl  >(decl)) return DeclSerializer::SerializeVarTemplatePartialSpecializationDecl  (contextItems, vtpsd);              break;
              //case clang::Decl::Kind::ClassTemplatePartialSpecialization: if (const ClassTemplatePartialSpecializationDecl* ctpsd = dyn_cast<ClassTemplatePartialSpecializationDecl>(decl)) return DeclSerializer::SerializeClassTemplatePartialSpecializationDecl(contextItems, ctpsd);              break;
              //case clang::Decl::Kind::ClassTemplateSpecialization:        if (const ClassTemplateSpecializationDecl*         ctsd = dyn_cast<ClassTemplateSpecializationDecl       >(decl)) return DeclSerializer::SerializeClassTemplateSpecializationDecl       (contextItems, ctsd);               break;
              //case clang::Decl::Kind::ClassTemplate:                      if (const ClassTemplateDecl*                        ctd = dyn_cast<ClassTemplateDecl                     >(decl)) return DeclSerializer::SerializeClassTemplateDecl                     (contextItems, ctd);                break;
              //case clang::Decl::Kind::FunctionTemplate:                   if (const FunctionTemplateDecl *                    ftd = dyn_cast<FunctionTemplateDecl                  >(decl)) return DeclSerializer::SerializeFunctionTemplateDecl                  (contextItems, ftd);                break;
                case clang::Decl::Kind::CXXMethod: // is a subclass of FunctionDecl
                case clang::Decl::Kind::Function:                           if (const FunctionDecl* functionDecl = dyn_cast<FunctionDecl>(decl)) return FunctionDeclSerializer<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr, true>(contextItems, functionDecl).Serialize(); break;
              //case clang::Decl::Kind::CXXConversion:                      if (const CXXConversionDecl*          cxxConversionDecl = dyn_cast<CXXConversionDecl                     >(decl)) return DeclSerializer::SerializeCXXConversionDecl                     (contextItems, cxxConversionDecl);  break;
              //case clang::Decl::Kind::CXXConstructor:                     if (const CXXConstructorDecl*        cxxConstructorDecl = dyn_cast<CXXConstructorDecl                    >(decl)) return DeclSerializer::SerializeCXXConstructorDecl                    (contextItems, cxxConstructorDecl); break;
              //case clang::Decl::Kind::CXXDestructor:                      if (const CXXDestructorDecl*          cxxDestructorDecl = dyn_cast<CXXDestructorDecl                     >(decl)) return DeclSerializer::SerializeCXXDestructorDecl                     (contextItems, cxxDestructorDecl);  break;
              //case clang::Decl::Kind::ParmVar:                            if (const ParmVarDecl *                             pvd = dyn_cast<ParmVarDecl                           >(decl)) return DeclSerializer::SerializeParmVarDecl                           (contextItems, pvd);                break;
                case clang::Decl::Kind::CXXRecord:                          if (const CXXRecordDecl* cxxRecordDecl = dyn_cast<CXXRecordDecl>(decl)) return CXXRecordDeclSerializer<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr, true>(contextItems, cxxRecordDecl).Serialize(); break;
              //case clang::Decl::Kind::Field:                              if (const FieldDecl *                         fieldDecl = dyn_cast<FieldDecl                             >(decl)) return DeclSerializer::SerializeFieldDecl                             (contextItems, fieldDecl);          break;
              //case clang::Decl::Kind::AccessSpec:                         if (const AccessSpecDecl *                   accessDecl = dyn_cast<AccessSpecDecl                        >(decl)) return DeclSerializer::SerializeAccessSpecDecl                        (contextItems, accessDecl);         break;
                case clang::Decl::Kind::Var:                                if (const VarDecl* varDecl = dyn_cast<VarDecl>(decl)) return VarDeclSerializer<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr, true>(contextItems, varDecl).Serialize(); break;
              //case clang::Decl::Kind::Enum:                               if (const EnumDecl*                            enumDecl = dyn_cast<EnumDecl                              >(decl)) return DeclSerializer::SerializeEnumDecl                              (contextItems, enumDecl);           break;
              //case clang::Decl::Kind::Typedef:                            if (const TypedefDecl *                     typedefDecl = dyn_cast<TypedefDecl                           >(decl)) return DeclSerializer::SerializeTypedefDecl                           (contextItems, typedefDecl);        break;
              //case clang::Decl::Kind::TypeAlias:                          if (const TypeAliasDecl *                           tad = dyn_cast<TypeAliasDecl                         >(decl)) return DeclSerializer::SerializeTypeAliasDecl                         (contextItems, tad);                break;
              //case clang::Decl::Kind::TypeAliasTemplate:                  if (const TypeAliasTemplateDecl*                   tatd = dyn_cast<TypeAliasTemplateDecl                 >(decl)) return DeclSerializer::SerializeTypeAliasTemplateDecl                 (contextItems, tatd);               break;
              //case clang::Decl::Kind::Friend:                             if (const FriendDecl *                       friendDecl = dyn_cast<FriendDecl                            >(decl)) return DeclSerializer::SerializeFriendDecl                            (contextItems, friendDecl);         break;
              //case clang::Decl::Kind::Concept:                            if (const ConceptDecl *                     conceptDecl = dyn_cast<ConceptDecl                           >(decl)) return DeclSerializer::SerializeConceptDecl                           (contextItems, conceptDecl);        break;
                default: break;
                }

                policy.FullyQualifiedName     = true;
                policy.SuppressUnwrittenScope = true;

                if (const auto* valueDecl = llvm::dyn_cast<clang::ValueDecl>(decl))
                    QualType(contextItems.context.getCanonicalType(valueDecl->getType())).print(os, policy, valueDecl->getName());
                else 
                if (const auto* typedefDecl = llvm::dyn_cast<clang::TypedefNameDecl>(decl))
                {
                    clang::QualType resolvedType = contextItems.context.getCanonicalType(typedefDecl->getUnderlyingType());
                    os << (llvm::isa<clang::TypeAliasDecl>(typedefDecl) ? "using " : "typedef ");
                    if (llvm::isa<clang::TypeAliasDecl>(typedefDecl))
                    {
                        os << typedefDecl->getName() << " = ";
                        resolvedType.print(os, policy);
                    } else
                        resolvedType.print(os, policy, typedefDecl->getName());
                } else
                if (const auto* tagDecl = llvm::dyn_cast<clang::TagDecl>(decl))
                    tagDecl->print(os, policy);
                else
                    decl->print(os, policy);
            }
            os.flush();
            return str + Semicolon::IfNeeded(str, decl);
        }
    }
}