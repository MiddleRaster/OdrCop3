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
        };

        class Can
        {
            static bool IsStaticInline(const clang::Decl* decl)
            {   // print() drops "inline" from "static inline" vars
                if (const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(decl))
                    if (varDecl->isInlineSpecified())
                        return true;
                return false;
            }
            static bool IsUnnamedUnion(const clang::Decl* decl)
            {
                if (const auto* nestedUnion = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
                    if (nestedUnion->isUnion() && nestedUnion->getName().empty())
                        return true;
                return false;
            }
            static bool IsUnnamedEnum(const clang::Decl* decl)
            {
                if (const auto* nestedEnum = llvm::dyn_cast<clang::EnumDecl>(decl))
                    if (nestedEnum->getName().empty())
                        return true;
                return false;
            }
            static bool IsTypedefOfUnnamedEnum(const clang::Decl* decl)
            {
                if (const auto* typedefDecl = llvm::dyn_cast<clang::TypedefDecl>(decl))
                    if (const auto* enumType = typedefDecl->getUnderlyingType()->getAs<clang::EnumType>())
                        if (enumType->getDecl()->getName().empty())
                            return true;
                return false;
            }

            template <typename Type> static bool PrintType(const ContextItems& contextItems, clang::QualType qualType)
            {
                if (const auto* type = qualType->getAs<Type>())
                    if (false == Can::Print(contextItems, type->getDecl()))
                        return false;
                return true;
            }

        public:
            static bool Print(const ContextItems& contextItems, const clang::Decl* decl)
            {
                if (decl->getKind() == clang::Decl::Kind::AccessSpec)
                    return false; // AccessSpecDecl::print() prints nothing

                if (NeedsManualSerialization(contextItems, decl) == true)
                    return false; // needs (anonymous namespace) type's definition inlined

                // recursively check declarations nested inside CXXRecordDecl*s (n levels deep)
                if (const auto* cxxRecordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
                    for (const clang::Decl* childDecl : cxxRecordDecl->decls())
                        if (false == Can::Print(contextItems, childDecl))
                            return false;

                // after recursion, see if field is printable (1 level deep)
                if (const auto* fieldDecl = llvm::dyn_cast<clang::FieldDecl>(decl))
                {
                    const clang::QualType fieldType = fieldDecl->getType();
                    if (false == Can::PrintType<clang::TypedefType>(contextItems, fieldType)) return false;
                    if (false == Can::PrintType<clang:: RecordType>(contextItems, fieldType)) return false;
                    if (false == Can::PrintType<clang::   EnumType>(contextItems, fieldType)) return false;
                }

                // after recursion (top-level)
                if (true == IsStaticInline(decl))
                    return false;
                if (true == IsUnnamedUnion(decl))
                    return false;
                if (true == IsUnnamedEnum(decl))
                    return false;
                if (true == IsTypedefOfUnnamedEnum(decl))
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

            using DeclSerializer = Serialize::Decl<&Decls<SerializeType, SerializeExpr>, SerializeType, SerializeExpr>;

            // Decl*s that must be serialized (e.g., for indenting)
            switch(decl->getKind())
            {
         // case clang::Decl::Kind::ClassTemplatePartialSpecialization: if (const ClassTemplatePartialSpecializationDecl* ctpsd = dyn_cast<ClassTemplatePartialSpecializationDecl>(decl)) return DeclSerializer::SerializeClassTemplatePartialSpecializationDecl(contextItems, ctpsd);              break;
         // case clang::Decl::Kind::ClassTemplateSpecialization:        if (const ClassTemplateSpecializationDecl*         ctsd = dyn_cast<ClassTemplateSpecializationDecl       >(decl)) return DeclSerializer::SerializeClassTemplateSpecializationDecl       (contextItems, ctsd);               break;
            case clang::Decl::Kind::ClassTemplate:                      if (const ClassTemplateDecl*                        ctd = dyn_cast<ClassTemplateDecl                     >(decl)) return DeclSerializer::SerializeClassTemplateDecl                     (contextItems, ctd);                break;
            default: break;
            }

            if (
             // false &&
                Can::Print(contextItems, decl))
            {
                struct Semicolon
                {
                    static std::string IfNeeded(const clang::Decl* decl)
                    {
                        switch (decl->getKind())
                        {
                        case clang::Decl::Kind::FunctionTemplate:
                        case clang::Decl::Kind::CXXMethod:
                        case clang::Decl::Kind::CXXConstructor:
                        case clang::Decl::Kind::CXXDestructor:
                        case clang::Decl::Kind::Function:        return "";
                        default:
                            return ";\n"; // structs so far
                        }
                    }
                };

                std::string str;
                llvm::raw_string_ostream os(str);
                decl->print(os, contextItems.printPolicy);
                os.flush();
                return str + Semicolon::IfNeeded(decl);
            }

            switch(decl->getKind())
            {
            case clang::Decl::Kind::CXXMethod:                          // is a subclass of FunctionDecl
            case clang::Decl::Kind::CXXDestructor:                      // so is this
            case clang::Decl::Kind::Function:                           if (const FunctionDecl*                    functionDecl = dyn_cast<FunctionDecl                          >(decl)) return DeclSerializer::SerializeFunctionDecl                          (contextItems, functionDecl);       break;
            case clang::Decl::Kind::CXXConversion:                      if (const CXXConversionDecl*          cxxConversionDecl = dyn_cast<CXXConversionDecl                     >(decl)) return DeclSerializer::SerializeCXXConversionDecl                     (contextItems, cxxConversionDecl);  break;
            case clang::Decl::Kind::CXXConstructor:                     if (const CXXConstructorDecl*        cxxConstructorDecl = dyn_cast<CXXConstructorDecl                    >(decl)) return DeclSerializer::SerializeCXXConstructorDecl                    (contextItems, cxxConstructorDecl); break;
            case clang::Decl::Kind::ParmVar:                            if (const ParmVarDecl *                             pvd = dyn_cast<ParmVarDecl                           >(decl)) return DeclSerializer::SerializeParmVarDecl                           (contextItems, pvd);                break;
            case clang::Decl::Kind::CXXRecord:                          if (const CXXRecordDecl *                           cxx = dyn_cast<CXXRecordDecl                         >(decl)) return DeclSerializer::SerializeCXXRecordDecl                         (contextItems, cxx);                break;
            case clang::Decl::Kind::Field:                              if (const FieldDecl *                         fieldDecl = dyn_cast<FieldDecl                             >(decl)) return DeclSerializer::SerializeFieldDecl                             (contextItems, fieldDecl);          break;
            case clang::Decl::Kind::FunctionTemplate:                   if (const FunctionTemplateDecl *                    ftd = dyn_cast<FunctionTemplateDecl                  >(decl)) return DeclSerializer::SerializeFunctionTemplateDecl                  (contextItems, ftd);                break;
            case clang::Decl::Kind::AccessSpec:                         if (const AccessSpecDecl *                   accessDecl = dyn_cast<AccessSpecDecl                        >(decl)) return DeclSerializer::SerializeAccessSpecDecl                        (contextItems, accessDecl);         break;
            case clang::Decl::Kind::Var:                                if (const VarDecl *                             varDecl = dyn_cast<VarDecl                               >(decl)) return DeclSerializer::SerializeVarDecl                               (contextItems, varDecl);            break;
            case clang::Decl::Kind::Enum:                               if (const EnumDecl*                            enumDecl = dyn_cast<EnumDecl                              >(decl)) return DeclSerializer::SerializeEnumDecl                              (contextItems, enumDecl);           break;
            case clang::Decl::Kind::Typedef:                            if (const TypedefDecl *                     typedefDecl = dyn_cast<TypedefDecl                           >(decl)) return DeclSerializer::SerializeTypedefDecl                           (contextItems, typedefDecl);        break;
            case clang::Decl::Kind::TypeAlias:                          if (const TypeAliasDecl *                           tad = dyn_cast<TypeAliasDecl                         >(decl)) return DeclSerializer::SerializeTypeAliasDecl                         (contextItems, tad);                break;
            case clang::Decl::Kind::TypeAliasTemplate:                  if (const TypeAliasTemplateDecl*                   tatd = dyn_cast<TypeAliasTemplateDecl                 >(decl)) return DeclSerializer::SerializeTypeAliasTemplateDecl                 (contextItems, tatd);               break;
            case clang::Decl::Kind::ClassTemplatePartialSpecialization: if (const ClassTemplatePartialSpecializationDecl* ctpsd = dyn_cast<ClassTemplatePartialSpecializationDecl>(decl)) return DeclSerializer::SerializeClassTemplatePartialSpecializationDecl(contextItems, ctpsd);              break;
            case clang::Decl::Kind::ClassTemplateSpecialization:        if (const ClassTemplateSpecializationDecl*         ctsd = dyn_cast<ClassTemplateSpecializationDecl       >(decl)) return DeclSerializer::SerializeClassTemplateSpecializationDecl       (contextItems, ctsd);               break;
            case clang::Decl::Kind::ClassTemplate:                      if (const ClassTemplateDecl*                       ctd = dyn_cast<ClassTemplateDecl                      >(decl)) return DeclSerializer::SerializeClassTemplateDecl                     (contextItems, ctd);                break;
            case clang::Decl::Kind::VarTemplateSpecialization:          if (const VarTemplateSpecializationDecl*           vtsd = dyn_cast<VarTemplateSpecializationDecl         >(decl)) return DeclSerializer::SerializeVarTemplateSpecializationDecl         (contextItems, vtsd);               break;
            case clang::Decl::Kind::VarTemplatePartialSpecialization:   if (const VarTemplatePartialSpecializationDecl*   vtpsd = dyn_cast<VarTemplatePartialSpecializationDecl  >(decl)) return DeclSerializer::SerializeVarTemplatePartialSpecializationDecl  (contextItems, vtpsd);              break;
            case clang::Decl::Kind::Friend:                             if (const FriendDecl *                       friendDecl = dyn_cast<FriendDecl                            >(decl)) return DeclSerializer::SerializeFriendDecl                            (contextItems, friendDecl);         break;
            default: break;
            }
            decl->dump();
            throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));
        }
    }
}