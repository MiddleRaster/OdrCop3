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

#include "SerializeFunctionDecl.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr, bool resolveNamespaceAliases=false> class CXXConversionDeclSerializer : private FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
        const ContextItems     & contextItems;
        const CXXConversionDecl* cxxConversionDecl;

        std::string get_ReturnType() const
        {
            if (IsType::EventuallyArrayOrFunctionPointer(cxxConversionDecl->getReturnType()))
            {
                // if a function returning a reference to an array, the syntax is tricky:
                // int (&ReturningReferenceTo1DArrayOfInts(int,double) noexcept)[3] { return blah; }
                // Everything from the "int" to the closing ) before the "[3]" goes into aux.
                std::string aux = this->SerializeFromCallingConventionToTrailingReturn([&]() { return ""; }, [&]() { return get_FunctionName(); });
                return TrimRightIf(aux, " ");
            }
            return {};
        }
        std::string get_FunctionName() const
        {
            clang::QualType qualType = cxxConversionDecl->getConversionType();
            if (NeedsManualSerialization(contextItems, qualType))
            {
                while (const clang::TypedefType* const typedefType = qualType->getAs<clang::TypedefType>())
                    qualType = typedefType->getDecl()->getUnderlyingType();
                return "operator " + IndentBlock(SerializeType(contextItems, qualType), 9);
            }

            std::string typeName;
            llvm::raw_string_ostream os(typeName);
            PrintingPolicy policy{contextItems.printPolicy};
            policy.FullyQualifiedName = resolveNamespaceAliases;
            cxxConversionDecl->getConversionType().print(os, policy);
            return "operator " + typeName;
        }
    public:
        CXXConversionDeclSerializer(const ContextItems& contextItems, const CXXConversionDecl* cxxConversionDecl)
            : FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, dyn_cast<FunctionDecl>(cxxConversionDecl))
            , contextItems(contextItems), cxxConversionDecl(cxxConversionDecl)
        {}
        std::string Serialize() const
        {
            return FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize([&](){ return get_ReturnType(); }, [&](){ return get_FunctionName(); });
        }
    };
}