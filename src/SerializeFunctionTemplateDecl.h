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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class FunctionTemplateDeclSerializer
    {
        const ContextItems& contextItems;
        const FunctionTemplateDecl* functionTemplateDecl;
    public:
        FunctionTemplateDeclSerializer(const ContextItems& contextItems, const FunctionTemplateDecl* functionTemplateDecl) : contextItems(contextItems), functionTemplateDecl(functionTemplateDecl) {}
        std::string Serialize() const
        {
            std::string out = GetTemplateHeader<SerializeDecl, SerializeType, SerializeExpr>(contextItems, functionTemplateDecl->getTemplateParameters());
            out += IndentBlock(SerializeDecl(contextItems, functionTemplateDecl->getTemplatedDecl()), LengthOfLastLine(out));
            return out + "\n";
        }
    };
}