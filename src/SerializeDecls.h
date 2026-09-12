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
#include "SerializeUsingDecl.h"

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

        class Print
        {
            static std::string AddSemiColonIfNeeded(const std::string& str, const clang::Decl* decl)
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
        public:
            static std::string Decl(const ContextItems& contextItems, const Decl* decl)
            {
                std::string str;
                llvm::raw_string_ostream os(str);
                if (const TemplateDecl* templateDecl = decl->getDescribedTemplate(); templateDecl && !contextItems.suppressTemplatePrefix)
                    templateDecl->print(os, contextItems.printPolicy);
                else
                    decl->print(os, contextItems.printPolicy);
                os.flush();
                return str + AddSemiColonIfNeeded(str, decl);
            }
        };

        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr, bool resolveNamespaceAliases>
        static inline std::string CallSerializer(const ContextItems& contextItems, const clang::Decl* decl)
        {
            switch(decl->getKind())
            {
            case clang::Decl::Kind::VarTemplateSpecialization:          if (const VarTemplateSpecializationDecl*           vtsd = dyn_cast<         VarTemplateSpecializationDecl>(decl)) return          VarTemplateSpecializationDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,               vtsd).Serialize(); break;
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
            case clang::Decl::Kind::TypeAlias:                          if (const TypeAliasDecl *                           tad = dyn_cast<                         TypeAliasDecl>(decl)) return                          TypeAliasDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,                tad).Serialize(); break;
            case clang::Decl::Kind::TypeAliasTemplate:                  if (const TypeAliasTemplateDecl*                   tatd = dyn_cast<                 TypeAliasTemplateDecl>(decl)) return                  TypeAliasTemplateDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems,               tatd).Serialize(); break;
            case clang::Decl::Kind::Friend:                             if (const FriendDecl *                       friendDecl = dyn_cast<                            FriendDecl>(decl)) return                             FriendDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,         friendDecl).Serialize(); break;
            case clang::Decl::Kind::Concept:                            if (const ConceptDecl *                     conceptDecl = dyn_cast<                           ConceptDecl>(decl)) return                            ConceptDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,        conceptDecl).Serialize(); break;
            case clang::Decl::Kind::Using:                              if (const UsingDecl *                         usingDecl = dyn_cast<                             UsingDecl>(decl)) return                              UsingDeclSerializer<SerializeDecl, SerializeType, SerializeExpr                         >(contextItems,          usingDecl).Serialize(); break;
            default: break;
            }
            // when this is released, comment out the next two lines, so that it won't throw but will print something
            decl->dump();
            throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));
            return Print::Decl(contextItems, decl);
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

            return Print::Decl(contextItems, decl);
        }
    }
}