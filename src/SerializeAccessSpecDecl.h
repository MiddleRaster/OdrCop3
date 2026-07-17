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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class AccessSpecDeclSerializer
    {
        const ContextItems  & contextItems;
        const AccessSpecDecl* accessSpecDecl;
    public:
        AccessSpecDeclSerializer(const ContextItems& contextItems, const AccessSpecDecl* accessSpecDecl) : contextItems(contextItems), accessSpecDecl(accessSpecDecl) {}

        std::string Serialize() const
        {
            switch (accessSpecDecl->getAccess())
            {
            case AS_public:    return "public:\n";
            case AS_protected: return "protected\n:";
            case AS_private:   return "private:\n";
            default:           return "";
            }
        }
    };
}