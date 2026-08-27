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
#include "SerializeRValueReferenceType.h"
#include "SerializeDecayedType.h"
#include "SerializeConstantArrayType.h"
#include "SerializeDependentSizedArrayType.h"
#include "SerializeParenType.h"
#include "SerializeMemberPointerType.h"
#include "SerializeEnumType.h"
#include "SerializeTypedefType.h"
#include "SerializeTemplateSpecializationType.h"
#include "SerializeBuiltinType.h"
#include "SerializeAutoType.h"
#include "SerializeSubstTemplateTypeParmType.h"
#include "SerializeDecltypeType.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr>
        struct Type
        {
            static std::string SerializeRecordType                (const ContextItems& contextItems, QualType qt, const                 RecordType*                 recordType) { return                 RecordTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                 recordType).Serialize(); }
            static std::string SerializeFunctionProtoType         (const ContextItems& contextItems, QualType qt, const          FunctionProtoType*          functionProtoType) { return          FunctionProtoTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,          functionProtoType).Serialize(); }
            static std::string SerializeTemplateTypeParmType      (const ContextItems& contextItems, QualType qt, const       TemplateTypeParmType*       templateTypeParmType) { return       TemplateTypeParmTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,       templateTypeParmType).Serialize(); }
            static std::string SerializeDependentNameType         (const ContextItems& contextItems, QualType qt, const          DependentNameType*          dependentNameType) { return          DependentNameTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,          dependentNameType).Serialize(); }
            static std::string SerializePointerType               (const ContextItems& contextItems, QualType qt, const                PointerType*                pointerType) { return                PointerTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                pointerType).Serialize(); }
            static std::string SerializeLValueReferenceType       (const ContextItems& contextItems, QualType qt, const        LValueReferenceType*        lValueReferenceType) { return        LValueReferenceTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,        lValueReferenceType).Serialize(); }
            static std::string SerializeRValueReferenceType       (const ContextItems& contextItems, QualType qt, const        RValueReferenceType*        rValueReferenceType) { return        RValueReferenceTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,        rValueReferenceType).Serialize(); }
            static std::string SerializeDecayedType               (const ContextItems& contextItems, QualType qt, const                DecayedType*                decayedType) { return                DecayedTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                decayedType).Serialize(); }
            static std::string SerializeParenType                 (const ContextItems& contextItems, QualType qt, const                  ParenType*                  parenType) { return                  ParenTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                  parenType).Serialize(); }
            static std::string SerializeMemberPointerType         (const ContextItems& contextItems, QualType qt, const          MemberPointerType*          memberPointerType) { return          MemberPointerTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,          memberPointerType).Serialize(); }
            static std::string SerializeEnumType                  (const ContextItems& contextItems, QualType qt, const                   EnumType*                   enumType) { return                   EnumTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                   enumType).Serialize(); }
            static std::string SerializeConstantArrayType         (const ContextItems& contextItems, QualType qt, const          ConstantArrayType*          constantArrayType) { return          ConstantArrayTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,          constantArrayType).Serialize(); }
            static std::string SerializeDependentSizedArrayType   (const ContextItems& contextItems, QualType qt, const    DependentSizedArrayType*    dependentSizedArrayType) { return    DependentSizedArrayTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,    dependentSizedArrayType).Serialize(); }
            static std::string SerializeTypedefType               (const ContextItems& contextItems, QualType qt, const                TypedefType*                typedefType) { return                TypedefTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                typedefType).Serialize(); }
            static std::string SerializeTemplateSpecializationType(const ContextItems& contextItems, QualType qt, const TemplateSpecializationType* templateSpecializationType) { return TemplateSpecializationTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt, templateSpecializationType).Serialize(); }
            static std::string SerializeBuiltinType               (const ContextItems& contextItems, QualType qt, const                BuiltinType*                builtinType) { return                BuiltinTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                builtinType).Serialize(); }
            static std::string SerializeAutoType                  (const ContextItems& contextItems, QualType qt, const                   AutoType*                   autoType) { return                   AutoTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,                   autoType).Serialize(); }
            static std::string SerializeSubstTemplateTypeParmType (const ContextItems& contextItems, QualType qt, const  SubstTemplateTypeParmType*  substTemplateTypeParmType) { return  SubstTemplateTypeParmTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,  substTemplateTypeParmType).Serialize(); }
            static std::string SerializeDecltypeType              (const ContextItems& contextItems, QualType qt, const               DecltypeType*               decltypeType) { return               DecltypeTypeSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt,               decltypeType).Serialize(); }
        };

        template<auto SerializeDecl, auto SerializeExpr>
        inline std::string Types(const ContextItems& contextItems, clang::QualType qualType)
        {
            struct Can
            {
                static bool Print(const ContextItems& contextItems, QualType qualType)
                {
                    qualType = qualType.getNonReferenceType(); // strips off references only

                    if (qualType->isArrayType())
                        if (const auto* arrayType = qualType->getAsArrayTypeUnsafe())
                            if (false == Can::Print(contextItems, arrayType->getElementType()))
                                return false;

                    if (const auto* pointerType = qualType->getAs<clang::PointerType>())
                        if (false == Can::Print(contextItems, pointerType->getPointeeType()))
                            return false;

                    if (const auto* parenType = qualType->getAs<clang::ParenType>())
                        if (false == Can::Print(contextItems, parenType->getInnerType()))
                            return false;

                    if (const auto* functionProtoType = qualType->getAs<clang::FunctionProtoType>())
                    {
                        if (false == Can::Print(contextItems, functionProtoType->getReturnType().getNonReferenceType()))
                            return false;

                        for (clang::QualType paramType : functionProtoType->param_types())
                            if (false == Can::Print(contextItems, paramType))
                                return false;
                    }

                    switch (qualType.getTypePtr()->getTypeClass())
                    { // some types must be manually serialized, no matter what. E.g., typedefs
                    case clang::Type::TypeClass::Typedef: return false;
                    case clang::Type::TypeClass::Enum:
                        if (const EnumType* enumType = dyn_cast<EnumType>(qualType.getTypePtr()))
                            if (enumType->getDecl()->getName().empty())
                                return false;
                        break;
                    case clang::Type::TypeClass::Record:
                        if (const RecordType* recordType = dyn_cast<RecordType>(qualType.getTypePtr()))
                            if (recordType->getDecl()->getName().empty())
                                return false;
                        break;
                    case clang::Type::TypeClass::TemplateSpecialization:
                        if (const auto* templateSpecializationType = qualType->getAs<clang::TemplateSpecializationType>())
                            for (const auto& arg : templateSpecializationType->template_arguments())
                                if (arg.getKind() == clang::TemplateArgument::ArgKind::Type)
                                    if (false == Can::Print(contextItems, arg.getAsType()))
                                        return false;
                        break;
                    case clang::Type::TypeClass::Decayed:
                        if (const auto* decayedType = qualType->getAs<clang::DecayedType>())
                            if (false == Can::Print(contextItems, decayedType->getOriginalType()))
                                return false;
                        break;
                    default: break;
                    }
                    if (NeedsManualSerialization(contextItems, qualType))
                        return false;
                    return true;
                }
            };
            if (Can::Print(contextItems, qualType) == false)
            {
                using TypeSerializer = Serialize::Type<SerializeDecl, &Types<SerializeDecl, SerializeExpr>, SerializeExpr>;
                switch (qualType.getTypePtr()->getTypeClass())
                {
                case clang::Type::TypeClass::Typedef:                if (const                TypedefType*                typedefType = dyn_cast<               TypedefType>(qualType.getTypePtr())) return TypeSerializer::SerializeTypedefType               (contextItems, qualType,                typedefType); break;
                case clang::Type::TypeClass::Enum:                   if (const                   EnumType*                   enumType = dyn_cast<                  EnumType>(qualType.getTypePtr())) return TypeSerializer::SerializeEnumType                  (contextItems, qualType,                   enumType); break;
                case clang::Type::TypeClass::FunctionProto:          if (const          FunctionProtoType*          functionProtoType = dyn_cast<         FunctionProtoType>(qualType.getTypePtr())) return TypeSerializer::SerializeFunctionProtoType         (contextItems, qualType,          functionProtoType); break;
                case clang::Type::TypeClass::Paren:                  if (const                  ParenType*                  parenType = dyn_cast<                 ParenType>(qualType.getTypePtr())) return TypeSerializer::SerializeParenType                 (contextItems, qualType,                  parenType); break;
                case clang::Type::TypeClass::Record:                 if (const                 RecordType*                 recordType = dyn_cast<                RecordType>(qualType.getTypePtr())) return TypeSerializer::SerializeRecordType                (contextItems, qualType,                 recordType); break;
                case clang::Type::TypeClass::TemplateTypeParm:       if (const       TemplateTypeParmType*       templateTypeParmType = dyn_cast<      TemplateTypeParmType>(qualType.getTypePtr())) return TypeSerializer::SerializeTemplateTypeParmType      (contextItems, qualType,       templateTypeParmType); break;
                case clang::Type::TypeClass::DependentName:          if (const          DependentNameType*          dependentNameType = dyn_cast<         DependentNameType>(qualType.getTypePtr())) return TypeSerializer::SerializeDependentNameType         (contextItems, qualType,          dependentNameType); break;
                case clang::Type::TypeClass::Pointer:                if (const                PointerType*                pointerType = dyn_cast<               PointerType>(qualType.getTypePtr())) return TypeSerializer::SerializePointerType               (contextItems, qualType,                pointerType); break;
                case clang::Type::TypeClass::LValueReference:        if (const        LValueReferenceType*        lValueReferenceType = dyn_cast<       LValueReferenceType>(qualType.getTypePtr())) return TypeSerializer::SerializeLValueReferenceType       (contextItems, qualType,        lValueReferenceType); break;
                case clang::Type::TypeClass::RValueReference:        if (const        RValueReferenceType*        rValueReferenceType = dyn_cast<       RValueReferenceType>(qualType.getTypePtr())) return TypeSerializer::SerializeRValueReferenceType       (contextItems, qualType,        rValueReferenceType); break;
                case clang::Type::TypeClass::Decayed:                if (const                DecayedType*                decayedType = dyn_cast<               DecayedType>(qualType.getTypePtr())) return TypeSerializer::SerializeDecayedType               (contextItems, qualType,                decayedType); break;
                case clang::Type::TypeClass::MemberPointer:          if (const          MemberPointerType*          memberPointerType = dyn_cast<         MemberPointerType>(qualType.getTypePtr())) return TypeSerializer::SerializeMemberPointerType         (contextItems, qualType,          memberPointerType); break;
                case clang::Type::TypeClass::ConstantArray:          if (const          ConstantArrayType*          constantArrayType = dyn_cast<         ConstantArrayType>(qualType.getTypePtr())) return TypeSerializer::SerializeConstantArrayType         (contextItems, qualType,          constantArrayType); break;
                case clang::Type::TypeClass::DependentSizedArray:    if (const    DependentSizedArrayType*    dependentSizedArrayType = dyn_cast<   DependentSizedArrayType>(qualType.getTypePtr())) return TypeSerializer::SerializeDependentSizedArrayType   (contextItems, qualType,    dependentSizedArrayType); break;
                case clang::Type::TypeClass::TemplateSpecialization: if (const TemplateSpecializationType* templateSpecializationType = dyn_cast<TemplateSpecializationType>(qualType.getTypePtr())) return TypeSerializer::SerializeTemplateSpecializationType(contextItems, qualType, templateSpecializationType); break;
                case clang::Type::TypeClass::Builtin:                if (const                BuiltinType*                builtinType = dyn_cast<               BuiltinType>(qualType.getTypePtr())) return TypeSerializer::SerializeBuiltinType               (contextItems, qualType,                builtinType); break;
                case clang::Type::TypeClass::Auto:                   if (const                   AutoType*                   autoType = dyn_cast<                  AutoType>(qualType.getTypePtr())) return TypeSerializer::SerializeAutoType                  (contextItems, qualType,                   autoType); break;
                case clang::Type::TypeClass::SubstTemplateTypeParm:  if (const SubstTemplateTypeParmType *  substTemplateTypeParmType = dyn_cast< SubstTemplateTypeParmType>(qualType.getTypePtr())) return TypeSerializer::SerializeSubstTemplateTypeParmType (contextItems, qualType,  substTemplateTypeParmType); break;
                case clang::Type::TypeClass::Decltype:               if (const               DecltypeType*               decltypeType = dyn_cast<              DecltypeType>(qualType.getTypePtr())) return TypeSerializer::SerializeDecltypeType              (contextItems, qualType,               decltypeType); break;
                default:
                    break;
                };
                // when released, comment out the following 2 lines, so that we print() something
                qualType.dump();
                throw OdrCop3::UnhandledException(std::string("unhandled type::getTypeClass: ") + enum_name(qualType.getTypePtr()->getTypeClass()));
            }

            std::string str;
            llvm::raw_string_ostream os(str);
            qualType.print(os, contextItems.printPolicy, contextItems.aux);
            os.flush();
            return str;
        }
    }
}