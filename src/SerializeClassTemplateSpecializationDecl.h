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

#include "SerializeCXXRecordDecl.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr, bool resolveNamespaceAliases=false> class ClassTemplateSpecializationDeclSerializer
    {
        const ContextItems& contextItems;
        const ClassTemplateSpecializationDecl* classTemplateSpecializationDecl;
    public:
        ClassTemplateSpecializationDeclSerializer(const ContextItems& contextItems, const ClassTemplateSpecializationDecl* classTemplateSpecializationDecl) : contextItems(contextItems), classTemplateSpecializationDecl(classTemplateSpecializationDecl) {}
        std::string Serialize() const
        {
            std::string out = "template<> ";
            // a classTemplateSpecializationDecl* "is a" CXXRecordDecl, so I can't call SerializeDecl, as the RecursionPreventor will kick in. So, call the right serializer directly.
            out += CXXRecordDeclSerializer<SerializeDecl, SerializeType, SerializeExpr, resolveNamespaceAliases>(contextItems.withAux(TemplateArgsToString(contextItems, classTemplateSpecializationDecl)),
                                                                                                                 static_cast<const clang::CXXRecordDecl*>(classTemplateSpecializationDecl)).Serialize();
            return out;
        }
    };
}
