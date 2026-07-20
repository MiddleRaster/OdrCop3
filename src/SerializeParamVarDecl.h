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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class ParmVarDeclSerializer
    {
        const ContextItems& contextItems;
        const ParmVarDecl* parmVarDecl;
    public:
        ParmVarDeclSerializer(const ContextItems& contextItems, const ParmVarDecl* parmVarDecl) : contextItems(contextItems), parmVarDecl(parmVarDecl) {}

        std::string Serialize() const
        {
            std::string out;

            {   // leading attributes
                if (TypeSourceInfo* typeSourceInfo = parmVarDecl->getTypeSourceInfo())
                {
                    SourceLocation typeLoc = typeSourceInfo->getTypeLoc().getBeginLoc();
                    for (const Attr* attr : parmVarDecl->attrs())
                    {
                        if (attr->getLocation() > typeLoc)
                            continue; // this is a trailing attribute
                        out += SerializeAttr(contextItems, attr);
                    }
                }
            }

            // type
            if (true == ContainsAnonymousType(parmVarDecl->getType()))
            {
                out += IndentBlock(SerializeType(contextItems, parmVarDecl->getType()), out.size() - (out.rfind('\n')+1));
                out  = TrimRightIf(out, ";");
            } else
                out += parmVarDecl->getType().getAsString(contextItems.printPolicy);

            // name if any
            if (parmVarDecl->getIdentifier())
                out += " " + parmVarDecl->getName().str(); // name

            // default value if any
            if (parmVarDecl->hasDefaultArg())
            {   // Default argument, if any
                std::string s;
                llvm::raw_string_ostream os(s);
                parmVarDecl->getDefaultArg()->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();
                out += " = " + s;
            }

            {   // trailing attributes
                if (TypeSourceInfo* typeSourceInfo = parmVarDecl->getTypeSourceInfo())
                {
                    SourceLocation typeLoc = typeSourceInfo->getTypeLoc().getBeginLoc();
                    for (const Attr* attr : parmVarDecl->attrs())
                    {
                        if (attr->getLocation() < typeLoc)
                            continue; // leading attribute
                        out += " " + SerializeAttr(contextItems, attr);
                        out  = TrimRightIf(out, " ");
                    }
                }
            }
            return out;
        }
    };
}