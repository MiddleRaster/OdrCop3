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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class UnresolvedLookupExprSerializer
    {
        const ContextItems        & contextItems;
        const UnresolvedLookupExpr* unresolvedLookupExpr;
    public:
        UnresolvedLookupExprSerializer(const ContextItems& contextItems, const UnresolvedLookupExpr* unresolvedLookupExpr) : contextItems(contextItems), unresolvedLookupExpr(unresolvedLookupExpr) {}
        std::string Serialize() const
        {
            std::string out;
            out += unresolvedLookupExpr->getName().getAsString();

            if (unresolvedLookupExpr->hasExplicitTemplateArgs())
            {
                out += "<";
                bool first = true;
                for (const clang::TemplateArgumentLoc& argLoc : unresolvedLookupExpr->template_arguments())
                {
                    if (first)
                        first = false;
                    else
                        out += ", ";
                    out += SerializeTemplateArgument<SerializeDecl, SerializeType, SerializeExpr>(contextItems, argLoc.getArgument(), LengthOfLastLine(out));
                }
                out += ">";
            }
            return out;
        }
    };
}