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
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class ParmVarDeclSerializer
    {
        const ContextItems& contextItems;
        const ParmVarDecl* parmVarDecl;
    public:
        ParmVarDeclSerializer(const ContextItems& contextItems, const ParmVarDecl* parmVarDecl) : contextItems(contextItems), parmVarDecl(parmVarDecl) {}

        std::string Serialize() const
        {
            std::string out;
            if (TypeSourceInfo* typeSourceInfo = parmVarDecl->getTypeSourceInfo())
            {   // leading attributes
                SourceLocation typeLoc = typeSourceInfo->getTypeLoc().getBeginLoc();
                for (const Attr* attr : parmVarDecl->attrs())
                    if (attr->getLocation() <= typeLoc)
                        out += SerializeAttr(contextItems, attr);
            }
            auto startOfParm = out.size();

            out += IndentBlock(SerializeType(contextItems, parmVarDecl->getType()), LengthOfLastLine(out));
            out  = TrimRightIf(out, ";");
            if((out.substr(out.size()-1) != "*")&& // e.g., int *i = nullptr (no space between * and i)
               (out.substr(out.size()-1) != "&") ) // e.g., const T &v       (no space between & and v)
                out += " ";                        // e.g., T a               (a space between T and a)

            if (parmVarDecl->getIdentifier()) // name if any
                out += parmVarDecl->getName().str();
            else
                out = TrimRightIf(out, " "); // don't want the space if nameless

            // slighly hilarious special case:  Decl::print() prints ellipsis one way, while Type::print() prints it the other. I'm going with "SomeType ...args". 
            if (parmVarDecl->isParameterPack())
            {
                std::size_t pos = out.find("... ", startOfParm);
                if (pos != std::string::npos)
                    out.replace(pos, 4, " ...");
            }

            if (parmVarDecl->hasDefaultArg()) // default argument, if any
            {
                std::string s;
                llvm::raw_string_ostream os(s);
                parmVarDecl->getDefaultArg()->printPretty(os, nullptr, contextItems.printPolicy);
                os.flush();
                out += " = " + s;
            }
            if (TypeSourceInfo* typeSourceInfo = parmVarDecl->getTypeSourceInfo())
            {   // trailing attributes
                SourceLocation typeLoc = typeSourceInfo->getTypeLoc().getBeginLoc();
                for (const Attr* attr : parmVarDecl->attrs())
                    if (attr->getLocation() >= typeLoc)
                        out += " " + TrimRightIf(SerializeAttr(contextItems, attr), " ");
            }
            return out;
        }
    };
}