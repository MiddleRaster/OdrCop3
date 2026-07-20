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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class FunctionProtoTypeSerializer
    {
        const ContextItems      & contextItems;
        const FunctionProtoType * functionProtoType;
        QualType qt;
    public:
        FunctionProtoTypeSerializer(const ContextItems& contextItems, QualType qt, const FunctionProtoType* functionProtoType) : contextItems(contextItems), qt(qt), functionProtoType(functionProtoType) {}

        bool IsItAnonymous(QualType qualType) const
        {
            for (;;)
            {
                if (qualType->isPointerType()         ||
                    qualType->isReferenceType()       ||
                    qualType->isMemberPointerType()   ||
                    qualType->isFunctionPointerType() ||
                    qualType->isFunctionReferenceType())
                    qualType = qualType->getPointeeType();
                else
                if (qualType->isArrayType())
                    qualType = QualType(qualType->getArrayElementTypeNoTypeQual(), 0);
                else
                    break;
            }

            const Decl * decl = nullptr;
            if (const clang::RecordType* recordType = qualType->getAs<clang::RecordType>())
                decl = recordType->getDecl();
            if (const clang::EnumType* enumType = qualType->getAs<clang::EnumType  >())
                decl = enumType->getDecl();

            if (decl)
                return IsDefinedInAnonymousNamespace(OdrCop3::StripPointersAndReferences(decl));
            return false;
        }

        std::string Output(QualType qualType) const
        {
            if (IsItAnonymous(qualType))
                return SerializeType(contextItems, qualType);

            std::string s;
            llvm::raw_string_ostream os(s);
            qualType.print(os, contextItems.printPolicy);
            os.flush();
            return s;
        }
        std::string get_ReturnType() const { return Output(functionProtoType->getReturnType()); }
        std::string get_Parameters() const
        {
            std::string out;

            bool first = true;
            for (clang::QualType qualType : functionProtoType->param_types())
            {
                if (first)
                    first = false;
                else
                    out += ", ";

                out += Output(qualType);
            }
            return out;
        }

        std::string Serialize() const
        { // e.g., for "void (*callback2)(Foo*);", QualType::print() returns "void (Foo *)"
            std::string out;
            out += get_ReturnType();
            out  = TrimRightIf(out, "\n");
            out  = TrimRightIf(out, ";");

            out += contextItems.aux; // I don't particularly like this design... but this is how I insert " (*callback2)"
            out += "(";
            out += IndentBlock(get_Parameters(), out.size() - (out.rfind('\n') + 1));
            out += ")";
            return out;
        }
    };
}