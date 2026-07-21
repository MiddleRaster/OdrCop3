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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class MemberPointerTypeSerializer
    {
        const ContextItems     & contextItems;
        const MemberPointerType* memberPointerType;
        QualType qt;

        std::string GetQualifierIfAnonymousNamespace() const
        {
            std::string out;

            const clang::NestedNameSpecifier& qualifier = memberPointerType->getQualifier();
            if (const clang::Type* type = qualifier.getAsType())
            {
                clang::QualType qt(type, 0);
                qt = qt.getCanonicalType();
                if (const auto* recordType = qt->getAs<clang::RecordType>())
                {
                    const clang::RecordDecl* recordDecl = recordType->getDecl();
                    if (IsDefinedInAnonymousNamespace(static_cast<const Decl*>(recordDecl)))
                    {
                        out += IndentBlock(SerializeDecl(contextItems, memberPointerType->getQualifier().getAsRecordDecl()), out.size());
                        out = TrimRightIf(out, ";");
                    }
                }
            }
            return out;
        }
    public:
        MemberPointerTypeSerializer(const ContextItems& contextItems, QualType qt, const MemberPointerType* memberPointerType) : contextItems(contextItems), qt(qt), memberPointerType(memberPointerType) {}
        std::string Serialize() const
        {
            std::string qualifierName;
            int indentation = 0;

            std::string anonQualifier = GetQualifierIfAnonymousNamespace();
            if (anonQualifier.find("\n") != std::string::npos)
            {   // if multiline, do this twice: first to figure out what the indentation needs to be; then again with the right indentation
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, " (?????)");
                std::string placeHolder = SerializeType(ci2, memberPointerType->getPointeeType());
                indentation = static_cast<int>(placeHolder.find("?????"));
                qualifierName = anonQualifier;
            }
            else
            {
                std::string s;
                llvm::raw_string_ostream os(s);
                memberPointerType->getQualifier().print(os, contextItems.printPolicy);
                os.flush();
                qualifierName = s;
            }

            ContextItems ci(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, " (" + qualifierName + "::*)");
            std::string out = IndentBlock(SerializeType(ci, memberPointerType->getPointeeType()), indentation);
            return out;
        }
    };
}