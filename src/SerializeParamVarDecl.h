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
                        out = out.substr(0, out.size() - 1); // strip off last " "
                    }
                }
            }
            return out;

            //IndirectionCvStripper ics(param->getType().getCanonicalType());

            //const auto* recordType = dyn_cast<clang::RecordType>(ics.GetBaseType().getTypePtr());
            //if (recordType && recordType->getDecl()->isInAnonymousNamespace())
            //{
            //    fqn += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())),
            //        fqn.size() - (fqn.rfind('\n') + 1) + ics.ConstructPrefix().size(),
            //        ics.ConstructPrefix());
            //    fqn = fqn.substr(0, fqn.size() - 2); // strip off last ";\n"
            //    fqn += ics.ConstructPointersAndReferences();
            //}
            //else if (recordType && dyn_cast<ClassTemplateSpecializationDecl>(recordType->getDecl()))
            //{
            //    const auto* spec = dyn_cast<ClassTemplateSpecializationDecl>(recordType->getDecl());
            //    fqn += ics.ConstructPrefix() + spec->getQualifiedNameAsString();
            //    fqn += IndentBlock(TemplateArgsToString(spec, false), fqn.size());
            //    fqn = fqn.substr(0, fqn.size() - 2); // strip last ";\n"
            //    fqn += ">" + ics.ConstructPointersAndReferences();
            //}
            //else if (const auto* enumTy = dyn_cast<clang::EnumType>(ics.GetBaseType().getTypePtr()); enumTy && enumTy->getDecl()->isInAnonymousNamespace()) {
            //    fqn += ics.ConstructPrefix() + ConstructEnumDefinition(enumTy->getDecl());
            //    fqn += ics.ConstructPointersAndReferences();
            //}
            //else
            //    fqn += param->getType().getAsString(printPolicy);

            //// append name, etc., as needed
            //fqn += ics.ConstructSuffixWithName(param->getName().str());

            //if (param->hasDefaultArg())
            //{   // Default argument, if any
            //    std::string out;
            //    llvm::raw_string_ostream os(out);
            //    param->getDefaultArg()->printPretty(os, nullptr, printPolicy);
            //    os.flush();
            //    fqn += " = " + out;
            //}
            //fqn += ", ";

        }
    };
}