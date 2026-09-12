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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class RequiresExprSerializer
    {
        const ContextItems& contextItems;
        const RequiresExpr* requiresExpr;
    public:
        RequiresExprSerializer(const ContextItems& contextItems, const RequiresExpr* requiresExpr) : contextItems(contextItems), requiresExpr(requiresExpr) {}
        std::string Serialize() const
        {
            std::string out;
            out += "requires ";

            { // parameters
                bool first = true;
                for (auto* parmVarDecl : requiresExpr->getLocalParameters())
                {
                    if (first) {
                        first = false;
                        out += "(";
                    } else
                        out += ", ";
                    out += IndentBlock(SerializeDecl(contextItems, parmVarDecl), LengthOfLastLine(out)); // or, should I get the type from the decl?
                    out  = TrimRightIf(out, ";");
                }
                if (first == false)
                    out += ") ";
            }
            { // requirements
                out += "{ ";

                for (const auto* requirement : requiresExpr->getRequirements())
                {
                    if (const auto* typeRequirement = llvm::dyn_cast<concepts::TypeRequirement>(requirement))
                    {
                        out += "typename ";
                        out += IndentBlock(SerializeType(contextItems, typeRequirement->getType()->getType()), LengthOfLastLine(out));
                        out += ";";
                        continue;
                    }
                    if (const auto* exprRequirement = llvm::dyn_cast<clang::concepts::ExprRequirement>(requirement))
                    {
                        if (exprRequirement->isSimple())
                        {
                            out += IndentBlock(SerializeExpr(contextItems, exprRequirement->getExpr()), LengthOfLastLine(out));
                            out += ";";
                        }
                        else
                        {
                            out += "{ ";
                            out += IndentBlock(SerializeExpr(contextItems, exprRequirement->getExpr()), LengthOfLastLine(out));
                            out += " }";

                            if (exprRequirement->hasNoexceptRequirement())
                                out += " noexcept";

                            const auto& returnTypeRequirement = exprRequirement->getReturnTypeRequirement();
                            if (!returnTypeRequirement.isEmpty())
                            {
                                out += " -> ";
                                out += IndentBlock(SerializeExpr(contextItems, returnTypeRequirement.getTypeConstraint()->getImmediatelyDeclaredConstraint()), LengthOfLastLine(out));
                            }
                            out += ";";
                        }
                        continue;
                    }
                    if (const auto* nestedRequirement = llvm::dyn_cast<clang::concepts::NestedRequirement>(requirement))
                    {
                        out += "requires ";
                        out += TrimRightIf(IndentBlock(SerializeExpr(contextItems, nestedRequirement->getConstraintExpr()), LengthOfLastLine(out)), " ");
                        out += ";";
                        continue;
                    }
                }
                out += " }";
            }
            return out;
        }
    };
}