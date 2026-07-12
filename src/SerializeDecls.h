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
#include <exception>

#include "SerializationUtils.h"
#include "SerializeFunctionDecl.h"
#include "SerializeCXXRecordDecl.h"
#include "SerializeEnumDecl.h"
#include "SerializeFieldDecl.h"
#include "SerializeVarDecl.h"
#include "SerializeParamVarDecl.h"
#include "SerializeAccessSpecDecl.h"
#include "magic_enum.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeType, auto SerializeAttr>
        struct Decl
        {
            static std::string SerializeFunctionTemplateDecl(const ContextItems& contextItems, const FunctionTemplateDecl* functionTemplateDecl) { return FunctionDeclSerializer  <SerializeDecl, SerializeType, SerializeAttr>(contextItems, functionTemplateDecl->getTemplatedDecl()).Serialize(); }
            static std::string SerializeFunctionDecl        (const ContextItems& contextItems, const FunctionDecl        *             funcDecl) { return FunctionDeclSerializer  <SerializeDecl, SerializeType, SerializeAttr>(contextItems,      funcDecl).Serialize(); }
            static std::string SerializeAccessSpecDecl      (const ContextItems& contextItems, const AccessSpecDecl      *           accessDecl) { return AccessSpecDeclSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems,    accessDecl).Serialize(); }
            static std::string SerializeFieldDecl           (const ContextItems& contextItems, const FieldDecl           *            fieldDecl) { return FieldDeclSerializer     <SerializeDecl, SerializeType, SerializeAttr>(contextItems,     fieldDecl).Serialize(); }
            static std::string SerializeVarDecl             (const ContextItems& contextItems, const VarDecl             *              varDecl) { return VarDeclSerializer       <SerializeDecl, SerializeType, SerializeAttr>(contextItems,       varDecl).Serialize(); }
            static std::string SerializeEnumDecl            (const ContextItems& contextItems, const EnumDecl            *             enumDecl) { return EnumDeclSerializer      <SerializeDecl, SerializeType, SerializeAttr>(contextItems,      enumDecl).Serialize(); }
            static std::string SerializeParmVarDecl         (const ContextItems& contextItems, const ParmVarDecl         *          parmVarDecl) { return ParmVarDeclSerializer   <SerializeDecl, SerializeType, SerializeAttr>(contextItems,   parmVarDecl).Serialize(); }
            static std::string SerializeCXXRecordDecl       (const ContextItems& contextItems, const CXXRecordDecl       *        cxxRecordDecl) { return CXXRecordDeclSerializer <SerializeDecl, SerializeType, SerializeAttr>(contextItems, cxxRecordDecl).Serialize(); }
        };

        template<auto SerializeType, auto SerializeAttr>
        static std::string Decls(const ContextItems& contextItems, const clang::Decl* decl)
        {
            if (!decl)
                throw std::invalid_argument("decl argument was null");

            using DeclSerializer = Serialize::Decl<&Decls<SerializeType, SerializeAttr>, SerializeType, SerializeAttr>;
            switch(decl->getKind())
            {
            case clang::Decl::Kind::CXXMethod:        // is a subclass of FunctionDecl
            case clang::Decl::Kind::CXXConstructor:   // so is this
            case clang::Decl::Kind::CXXConversion:    // and this
            case clang::Decl::Kind::Function:         if (const FunctionDecl* functionDecl = dyn_cast<FunctionDecl        >(decl)) return DeclSerializer::SerializeFunctionDecl  (contextItems, functionDecl); break;
            case clang::Decl::Kind::ParmVar:          if (const ParmVarDecl *          pvd = dyn_cast<ParmVarDecl         >(decl)) return DeclSerializer::SerializeParmVarDecl (contextItems, pvd);          break;
            case clang::Decl::Kind::CXXRecord:        if (const CXXRecordDecl*         cxx = dyn_cast<CXXRecordDecl       >(decl)) return DeclSerializer::SerializeCXXRecordDecl (contextItems, cxx);          break;
            case clang::Decl::Kind::Field:            if (const FieldDecl *      fieldDecl = dyn_cast<FieldDecl           >(decl)) return DeclSerializer::SerializeFieldDecl     (contextItems, fieldDecl);    break;
            case clang::Decl::Kind::FunctionTemplate: if (const FunctionTemplateDecl * ftd = dyn_cast<FunctionTemplateDecl>(decl)) return DeclSerializer::SerializeFunctionTemplateDecl(contextItems, ftd);    break;
            case clang::Decl::Kind::AccessSpec:       if (const AccessSpecDecl* accessDecl = dyn_cast<AccessSpecDecl      >(decl)) return DeclSerializer::SerializeAccessSpecDecl(contextItems, accessDecl);   break;
            case clang::Decl::Kind::Var:              if (const VarDecl *          varDecl = dyn_cast<VarDecl             >(decl)) return DeclSerializer::SerializeVarDecl       (contextItems, varDecl);      break;
            case clang::Decl::Kind::Enum:             if (const EnumDecl*         enumDecl = dyn_cast<EnumDecl            >(decl)) return DeclSerializer::SerializeEnumDecl      (contextItems, enumDecl);     break;
            default: break;
            }
            throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));
        }
    }
}