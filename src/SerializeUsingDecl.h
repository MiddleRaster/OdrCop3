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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class UsingDeclSerializer
    {
        const ContextItems& contextItems;
        const UsingDecl   * usingDecl;
    public:
        UsingDeclSerializer(const ContextItems& contextItems, const UsingDecl* usingDecl) : contextItems(contextItems), usingDecl(usingDecl) {}
        std::string Serialize() const
        {   // there should be no need to check NeedsManualSerialization. If we get here, we need it.
            for (auto* shadow : usingDecl->shadows())
            {
                if (auto* namedDecl = shadow->getTargetDecl())
                {
                    if (auto* cxxRecordDecl = dyn_cast<CXXRecordDecl>(namedDecl->getDeclContext()))
                    {
                        std::string out;
                        out += "using ";
                        out += TrimRightIf(IndentBlock(SerializeDecl(contextItems, cxxRecordDecl), 6), ";");
                        out += "::" + namedDecl->getNameAsString() + ";\n";
                        return out; // only 1, evidently
                    }
                }
            }
            // if all else fails
            std::string str;
            llvm::raw_string_ostream os(str);
            usingDecl->print(os, contextItems.printPolicy);
            os.flush();
            return str + ";\n";
        }
    };
}