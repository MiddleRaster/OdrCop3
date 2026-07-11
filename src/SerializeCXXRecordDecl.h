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

#include "SerializationUtils.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class CXXRecordDeclSerializer
    {
        const ContextItems & contextItems;
        const CXXRecordDecl* cxxRecordDecl;
    public:
        CXXRecordDeclSerializer(const ContextItems& contextItems, const CXXRecordDecl* cxxRecordDecl) : contextItems(contextItems), cxxRecordDecl(cxxRecordDecl) {}

//e.g., std::string get_ConstEval() const { return funcDecl->isConsteval() ? "consteval " : ""; }

        std::string Serialize() const
        {
            std::string fqn;

            return fqn;
        }
    };
}