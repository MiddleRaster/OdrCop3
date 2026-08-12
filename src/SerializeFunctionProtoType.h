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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class FunctionProtoTypeSerializer
    {
        const ContextItems      & contextItems;
        const FunctionProtoType * functionProtoType;
        QualType qt;

        std::string Output(QualType qualType) const
        {
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls); // strip off any aux (not for return value or args)
            std::string out = SerializeType(ci2, qualType);
            out = TrimRightIf(out, "\n");
            out = TrimRightIf(out, ";");
            return out;
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
                out += Output(qualType.getDesugaredType(contextItems.context));
            }
            return out;
        }
        std::string get_Variadic()   const
        {
            if (!functionProtoType->isVariadic())
                return "";
            if (functionProtoType->getNumParams() == 0)
                return "...";
            else
                return ",...";
        }
        std::string get_MethodQuals() const
        {
            std::string out;
            Qualifiers quals = functionProtoType->getMethodQuals();
            if (quals.hasConst())    out += " const";
            if (quals.hasVolatile()) out += " volatile";
            if (quals.hasRestrict()) out += " restrict";
            return out;
        }
        std::string get_RefQualifier() const
        {
            switch (functionProtoType->getRefQualifier())
            {
            case RQ_LValue: return " &";
            case RQ_RValue: return " &&";
            case RQ_None:
            default:        break;
            }
            return "";
        }
        std::string get_ExceptionSpecifier() const { return " " + GetExceptionSpecifier<SerializeDecl, SerializeType, SerializeExpr>(contextItems, functionProtoType, nullptr); }

    public:
        FunctionProtoTypeSerializer(const ContextItems& contextItems, QualType qt, const FunctionProtoType* functionProtoType) : contextItems(contextItems), qt(qt), functionProtoType(functionProtoType) {}
        std::string Serialize() const
        {   // e.g., for "void (*callback2)(Foo*);", QualType::print() returns "void (Foo *)"
            // important note:
            // this Serializer is called for both pointers-to-functions AND pointers-to-member-functions
            // So the ContextItems.aux field must be properly set up before calling this function.
            std::string out;
            out += get_ReturnType();
            out  = TrimRightIf(out, "\n");
            out  = TrimRightIf(out, ";");
            out  = TrimRightIf(out, " ");
            out += " " + contextItems.aux; // I like this design quite a bit:  this is how I insert (*callback2) or (S::*mp)
            out += "(";
            out += IndentBlock(get_Parameters(), LengthOfLastLine(out));
            out += get_Variadic();
            out += ")";
            out += get_MethodQuals();
            out += get_RefQualifier();
            out += TrimRightIf(get_ExceptionSpecifier(), " ");
            return out;
        }
    };
}