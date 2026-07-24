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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class ClassTemplatePartialSpecializationDeclSerializer
    {
        const ContextItems& contextItems;
        const ClassTemplatePartialSpecializationDecl* classTemplatePartialSpecializationDecl;
    public:
        ClassTemplatePartialSpecializationDeclSerializer(const ContextItems& contextItems, const ClassTemplatePartialSpecializationDecl* classTemplatePartialSpecializationDecl) : contextItems(contextItems), classTemplatePartialSpecializationDecl(classTemplatePartialSpecializationDecl) {}
        std::string Serialize() const
        {
            std::string args = TemplateArgsToString<SerializeDecl, SerializeType, SerializeAttr>(contextItems, classTemplatePartialSpecializationDecl);
            ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, args);

            std::string out;
            { // the "unspecialized" template parameters
                llvm::raw_string_ostream os(out);
                classTemplatePartialSpecializationDecl->getTemplateParameters()->print(os, contextItems.context, contextItems.printPolicy);
                os.flush();
            }

            // a classTemplatePartialSpecializationDecl* "is a" CXXRecordDecl, so I can't call SerializeDecl, as the RecursionPreventor will kick in. 
            // So, call the right serializer directly.
            out += IndentBlock(CXXRecordDeclSerializer<SerializeDecl, SerializeType, SerializeAttr>(ci2, static_cast<const clang::CXXRecordDecl*>(classTemplatePartialSpecializationDecl)).Serialize(), LengthOfLastLine(out)) + "\n";
            return out;
        }
    };
}
