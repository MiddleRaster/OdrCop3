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
#include "magic_enum.h"

#include "SerializeRecordType.h"
#include "SerializeFunctionProtoType.h"
#include "SerializeTemplateTypeParmType.h"
#include "SerializeDependentNameType.h"
#include "SerializePointerType.h"
#include "SerializeLValueReferenceType.h"
#include "SerializeDecayedType.h"
#include "SerializeConstantArrayType.h"
#include "SerializeDependentSizedArrayType.h"
#include "SerializeParenType.h"
#include "SerializeMemberPointerType.h"
#include "SerializeEnumType.h"
#include "SerializeTypedefType.h"
#include "SerializeTemplateSpecializationType.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeType, auto SerializeAttr>
        struct Type
        {
            static std::string SerializeRecordType                (const ContextItems& contextItems, QualType qt, const                 RecordType*                 recordType) { return                 RecordTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,                 recordType).Serialize(); }
            static std::string SerializeFunctionProtoType         (const ContextItems& contextItems, QualType qt, const          FunctionProtoType*          functionProtoType) { return          FunctionProtoTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,          functionProtoType).Serialize(); }
            static std::string SerializeTemplateTypeParmType      (const ContextItems& contextItems, QualType qt, const       TemplateTypeParmType*       templateTypeParmType) { return       TemplateTypeParmTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,       templateTypeParmType).Serialize(); }
            static std::string SerializeDependentNameType         (const ContextItems& contextItems, QualType qt, const          DependentNameType*          dependentNameType) { return          DependentNameTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,          dependentNameType).Serialize(); }
            static std::string SerializePointerType               (const ContextItems& contextItems, QualType qt, const                PointerType*                pointerType) { return                PointerTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,                pointerType).Serialize(); }
            static std::string SerializeLValueReferenceType       (const ContextItems& contextItems, QualType qt, const        LValueReferenceType*        lValueReferenceType) { return        LValueReferenceTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,        lValueReferenceType).Serialize(); }
            static std::string SerializeDecayedType               (const ContextItems& contextItems, QualType qt, const                DecayedType*                decayedType) { return                DecayedTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,                decayedType).Serialize(); }
            static std::string SerializeParenType                 (const ContextItems& contextItems, QualType qt, const                  ParenType*                  parenType) { return                  ParenTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,                  parenType).Serialize(); }
            static std::string SerializeMemberPointerType         (const ContextItems& contextItems, QualType qt, const          MemberPointerType*          memberPointerType) { return          MemberPointerTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,          memberPointerType).Serialize(); }
            static std::string SerializeEnumType                  (const ContextItems& contextItems, QualType qt, const                   EnumType*                   enumType) { return                   EnumTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,                   enumType).Serialize(); }
            static std::string SerializeConstantArrayType         (const ContextItems& contextItems, QualType qt, const          ConstantArrayType*          constantArrayType) { return          ConstantArrayTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,          constantArrayType).Serialize(); }
            static std::string SerializeDependentSizedArrayType   (const ContextItems& contextItems, QualType qt, const    DependentSizedArrayType*    dependentSizedArrayType) { return    DependentSizedArrayTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,    dependentSizedArrayType).Serialize(); }
            static std::string SerializeTypedefType               (const ContextItems& contextItems, QualType qt, const                TypedefType*                typedefType) { return                TypedefTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,                typedefType).Serialize(); }
            static std::string SerializeTemplateSpecializationType(const ContextItems& contextItems, QualType qt, const TemplateSpecializationType* templateSpecializationType) { return TemplateSpecializationTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt, templateSpecializationType).Serialize(); }
        };

        template<auto SerializeDecl, auto SerializeAttr>
        static std::string Types(const ContextItems& contextItems, clang::QualType qualType)
        {
            using TypeSerializer = Serialize::Type<SerializeDecl, &Types<SerializeDecl, SerializeAttr>, SerializeAttr>;
            switch (qualType.getTypePtr()->getTypeClass())
            {
            case clang::Type::TypeClass::Record:                 if (const                 RecordType*                 recordType = dyn_cast<                RecordType>(qualType.getTypePtr())) return TypeSerializer::SerializeRecordType                (contextItems, qualType,                 recordType); break;
            case clang::Type::TypeClass::FunctionProto:          if (const          FunctionProtoType*          functionProtoType = dyn_cast<         FunctionProtoType>(qualType.getTypePtr())) return TypeSerializer::SerializeFunctionProtoType         (contextItems, qualType,          functionProtoType); break;
            case clang::Type::TypeClass::TemplateTypeParm:       if (const       TemplateTypeParmType*       templateTypeParmType = dyn_cast<      TemplateTypeParmType>(qualType.getTypePtr())) return TypeSerializer::SerializeTemplateTypeParmType      (contextItems, qualType,       templateTypeParmType); break;
            case clang::Type::TypeClass::DependentName:          if (const          DependentNameType*          dependentNameType = dyn_cast<         DependentNameType>(qualType.getTypePtr())) return TypeSerializer::SerializeDependentNameType         (contextItems, qualType,          dependentNameType); break;
            case clang::Type::TypeClass::Pointer:                if (const                PointerType*                pointerType = dyn_cast<               PointerType>(qualType.getTypePtr())) return TypeSerializer::SerializePointerType               (contextItems, qualType,                pointerType); break;
            case clang::Type::TypeClass::LValueReference:        if (const        LValueReferenceType*        lValueReferenceType = dyn_cast<       LValueReferenceType>(qualType.getTypePtr())) return TypeSerializer::SerializeLValueReferenceType       (contextItems, qualType,        lValueReferenceType); break;
            case clang::Type::TypeClass::Decayed:                if (const                DecayedType*                decayedType = dyn_cast<               DecayedType>(qualType.getTypePtr())) return TypeSerializer::SerializeDecayedType               (contextItems, qualType,                decayedType); break;
            case clang::Type::TypeClass::Paren:                  if (const                  ParenType*                  parenType = dyn_cast<                 ParenType>(qualType.getTypePtr())) return TypeSerializer::SerializeParenType                 (contextItems, qualType,                  parenType); break;
            case clang::Type::TypeClass::MemberPointer:          if (const          MemberPointerType*          memberPointerType = dyn_cast<         MemberPointerType>(qualType.getTypePtr())) return TypeSerializer::SerializeMemberPointerType         (contextItems, qualType,          memberPointerType); break;
            case clang::Type::TypeClass::Enum:                   if (const                   EnumType*                   enumType = dyn_cast<                  EnumType>(qualType.getTypePtr())) return TypeSerializer::SerializeEnumType                  (contextItems, qualType,                   enumType); break;
            case clang::Type::TypeClass::ConstantArray:          if (const          ConstantArrayType*          constantArrayType = dyn_cast<         ConstantArrayType>(qualType.getTypePtr())) return TypeSerializer::SerializeConstantArrayType         (contextItems, qualType,          constantArrayType); break;
            case clang::Type::TypeClass::DependentSizedArray:    if (const    DependentSizedArrayType*    dependentSizedArrayType = dyn_cast<   DependentSizedArrayType>(qualType.getTypePtr())) return TypeSerializer::SerializeDependentSizedArrayType   (contextItems, qualType,    dependentSizedArrayType); break;
            case clang::Type::TypeClass::Typedef:                if (const                TypedefType*                typedefType = dyn_cast<               TypedefType>(qualType.getTypePtr())) return TypeSerializer::SerializeTypedefType               (contextItems, qualType,                typedefType); break;
            case clang::Type::TypeClass::TemplateSpecialization: if (const TemplateSpecializationType* templateSpecializationType = dyn_cast<TemplateSpecializationType>(qualType.getTypePtr())) return TypeSerializer::SerializeTemplateSpecializationType(contextItems, qualType, templateSpecializationType); break;
         // case clang::Type::TypeClass::InjectedClassName:
         // case clang::Type::TypeClass::PackExpansion:
         // case clang::Type::TypeClass::Builtin:
            default:
                break;
            };
            qualType.dump();
            throw OdrCop3::UnhandledException(std::string("unhandled type::getTypeClass: ") + enum_name(qualType.getTypePtr()->getTypeClass()));
        }
    }
}