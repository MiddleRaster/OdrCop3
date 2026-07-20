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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class FriendDeclSerializer
    {
        const ContextItems& contextItems;
        const FriendDecl  * friendDecl;
    public:
        FriendDeclSerializer(const ContextItems& contextItems, const FriendDecl* friendDecl) : contextItems(contextItems), friendDecl(friendDecl) {}
        std::string Serialize() const
        {
            if (const NamedDecl* namedDecl = friendDecl->getFriendDecl())
            {
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls);

                if (const FunctionTemplateDecl* functionTemplateDecl = dyn_cast<FunctionTemplateDecl>(namedDecl))
                {
                    if (const FunctionDecl* functionDecl = functionTemplateDecl->getTemplatedDecl())
                        ci2.wantFunctionBody = functionDecl->doesThisDeclarationHaveABody();
                    return SerializeDecl(ci2, functionTemplateDecl);
                }
                if (const FunctionDecl* functionDecl = dyn_cast<FunctionDecl>(namedDecl))
                {
                    ci2.wantFunctionBody = functionDecl->doesThisDeclarationHaveABody();
                    return SerializeDecl(ci2, functionDecl);
                }
                if (const ClassTemplateDecl* classTemplateDecl = dyn_cast<ClassTemplateDecl>(namedDecl))
                {
                    ci2.needsFriend = true;
                    return SerializeDecl(ci2, classTemplateDecl);
                }
                if (const CXXRecordDecl* cxxRecordDecl = dyn_cast<CXXRecordDecl>(namedDecl))
                    return SerializeDecl(contextItems, cxxRecordDecl);
            }
            if (const TypeSourceInfo* typeSourceInfo = friendDecl->getFriendType())
                return "friend " + SerializeType(contextItems, friendDecl->getFriendType()->getType()) + ";";

            return "";
        }
    };
}
