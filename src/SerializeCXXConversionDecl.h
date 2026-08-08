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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class CXXConversionDeclSerializer : private FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
        const ContextItems     & contextItems;
        const CXXConversionDecl* cxxConversionDecl;

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
            cxxConversionDecl->getConversionType().print(os, contextItems.printPolicy);
            return "operator " + typeName;
        }
    public:
        CXXConversionDeclSerializer(const ContextItems& contextItems, const CXXConversionDecl* cxxConversionDecl)
            : FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, dyn_cast<FunctionDecl>(cxxConversionDecl))
            , contextItems(contextItems), cxxConversionDecl(cxxConversionDecl)
        {}
        std::string Serialize() const
        {
            return FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize([&](){ return ""; }, [&](){ return get_FunctionName(); });
        }
    };
}