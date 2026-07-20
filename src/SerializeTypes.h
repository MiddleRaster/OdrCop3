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
#include "SerializePointerType.h"
#include "SerializeTemplateTypeParmType.h"
#include "SerializeDependentNameType.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeType, auto SerializeAttr>
        struct Type
        {
            static std::string SerializeRecordType          (const ContextItems& contextItems, QualType qt, const           RecordType*           recordType) { return           RecordTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,           recordType).Serialize(); }
            static std::string SerializeFunctionProtoType   (const ContextItems& contextItems, QualType qt, const    FunctionProtoType*    functionProtoType) { return    FunctionProtoTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,    functionProtoType).Serialize(); }
            static std::string SerializePointerType         (const ContextItems& contextItems, QualType qt, const          PointerType*          pointerType) { return          PointerTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,          pointerType).Serialize(); }
            static std::string SerializeTemplateTypeParmType(const ContextItems& contextItems, QualType qt, const TemplateTypeParmType* templateTypeParmType) { return TemplateTypeParmTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt, templateTypeParmType).Serialize(); }
            static std::string SerializeDependentNameType   (const ContextItems& contextItems, QualType qt, const    DependentNameType*    dependentNameType) { return    DependentNameTypeSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, qt,    dependentNameType).Serialize(); }
        };

        template<auto SerializeDecl, auto SerializeAttr>
        static std::string Types(const ContextItems& contextItems, clang::QualType qualType)
        {
            using TypeSerializer = Serialize::Type<SerializeDecl, &Types<SerializeDecl, SerializeAttr>, SerializeAttr>;
            switch (qualType.getTypePtr()->getTypeClass())
            {
            case clang::Type::TypeClass::Record:           if (const           RecordType*           recordType = dyn_cast<          RecordType>(qualType.getTypePtr())) return TypeSerializer::SerializeRecordType          (contextItems, qualType,           recordType); break;
            case clang::Type::TypeClass::FunctionProto:    if (const    FunctionProtoType*    functionProtoType = dyn_cast<   FunctionProtoType>(qualType.getTypePtr())) return TypeSerializer::SerializeFunctionProtoType   (contextItems, qualType,    functionProtoType); break;
            case clang::Type::TypeClass::Pointer:          if (const          PointerType*          pointerType = dyn_cast<         PointerType>(qualType.getTypePtr())) return TypeSerializer::SerializePointerType         (contextItems, qualType,          pointerType); break;
            case clang::Type::TypeClass::TemplateTypeParm: if (const TemplateTypeParmType* templateTypeParmType = dyn_cast<TemplateTypeParmType>(qualType.getTypePtr())) return TypeSerializer::SerializeTemplateTypeParmType(contextItems, qualType, templateTypeParmType); break;
            case clang::Type::TypeClass::DependentName:    if (const    DependentNameType*    dependentNameType = dyn_cast<   DependentNameType>(qualType.getTypePtr())) return TypeSerializer::SerializeDependentNameType   (contextItems, qualType,    dependentNameType); break;

         // case clang::Type::TypeClass::Builtin:
         // case clang::Type::TypeClass::Record:
         // case clang::Type::TypeClass::Enum:
         // case clang::Type::TypeClass::Typedef:
         // case clang::Type::TypeClass::TemplateSpecialization:
         // case clang::Type::TypeClass::InjectedClassName:
         // case clang::Type::TypeClass::PackExpansion:
            default:
                break;
            };
            throw OdrCop3::UnhandledException(std::string("unhandled type::getTypeClass: ") + enum_name(qualType.getTypePtr()->getTypeClass()));
        }
    }
}