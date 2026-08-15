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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ConceptSpecializationExprSerializer
    {
        const ContextItems             & contextItems;
        const ConceptSpecializationExpr* conceptSpecializationExpr;
    public:
        ConceptSpecializationExprSerializer(const ContextItems& contextItems, const ConceptSpecializationExpr* conceptSpecializationExpr) : contextItems(contextItems), conceptSpecializationExpr(conceptSpecializationExpr) {}
        std::string Serialize() const
        {
            std::string out;
            out += conceptSpecializationExpr->getNamedConcept()->getNameAsString();
            out += "<";

            for (const TemplateArgumentLoc& argLoc : conceptSpecializationExpr->getTemplateArgsAsWritten()->arguments())
            {
                out += SerializeTemplateArgument<SerializeDecl, SerializeType, SerializeExpr>(contextItems, argLoc.getArgument(), LengthOfLastLine(out));
                out += ", ";
            }
            out = TrimRightIf(out, ", ");
            out += "> ";

            return out;
        }
    };
}