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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class RecordTypeSerializer
    {
        const ContextItems& contextItems;
        const QualType    & qt;
        const RecordType  * recordType;
    public:
        RecordTypeSerializer(const ContextItems& contextItems, const QualType& qt) : contextItems(contextItems), qt(qt), recordType(dyn_cast<RecordType>(qt.getTypePtr()))
        {
            if (!recordType)
                throw OdrCop3::UnhandledException(std::string("bad dyn_cast in RecordTypeSerializer: ") + enum_name(qt.getTypePtr()->getTypeClass()));
        }

        std::string Serialize() const
        { // currently used for base types only. Will see how this unfolds in the future.

            //// when a base is defined in an anonymous namespace, include the full definition here.
            //const clang::Type* type = base.getType().getCanonicalType().getTypePtr();
            //const auto* recordType  = dyn_cast<RecordType>(type);
            //if (recordType && recordType->getDecl()->isInAnonymousNamespace())
            //{
            //    previousWasAnonymous = true;
            //    out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), out.size() - (out.rfind('\n') + 1));  // length of last line up to current spot
            //    out  = out.substr(0, out.size()-2); // strip off last ";\n"
            //}
            //else

            return qt.getAsString(contextItems.printPolicy);
        }
    };
}