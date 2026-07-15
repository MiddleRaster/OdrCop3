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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TypedefDeclSerializer
    {
        const ContextItems& contextItems;
        const TypedefDecl * typedefDecl;
    public:
        TypedefDeclSerializer(const ContextItems& contextItems, const TypedefDecl* typedefDecl) : contextItems(contextItems), typedefDecl(typedefDecl) {}

        std::string Serialize() const
        {
            QualType    underlying   = typedefDecl->getUnderlyingType().getCanonicalType();
            std::string aliasName    = typedefDecl->getQualifiedNameAsString();
            std::string resolvedType = underlying.getAsString(contextItems.printPolicy);

            // nameless or anonymous UDTs
            bool needsFullInlining = false;
            const RecordType* recordType = underlying->getAs<RecordType>();
            if (recordType != nullptr) {
                if (recordType->getDecl()->getName().empty())
                    needsFullInlining = true;
                else {
                    const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(recordType->getDecl()->getDeclContext());
                    if (nsDecl != nullptr)
                        if (nsDecl->isAnonymousNamespace())
                            needsFullInlining = true;
                }
                if (needsFullInlining)
                {
                    std::string fqtd = "using " + aliasName + " = ";
                    fqtd += IndentBlock(SerializeDecl(contextItems, dyn_cast<CXXRecordDecl>(recordType->getDecl())), fqtd.size());
                    fqtd  = fqtd.substr(0, fqtd.size()-2); // strip last ";\n"
                    fqtd += "; // typedef " + resolvedType + " " + aliasName + ";\n";
                    return fqtd;
                }
            }

            // nameless or anonymous enums
            bool isNameless           = false;
            bool isAnonymousNamespace = false;
            const EnumType * enumType = underlying->getAs<EnumType>();
            if (enumType != nullptr)
            {
                if (enumType->getDecl()->getName().empty())
                    isNameless = true;
                const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(enumType->getDecl()->getDeclContext());
                if (nsDecl != nullptr)
                    if (nsDecl->isAnonymousNamespace())
                        isAnonymousNamespace = true;
            }
            if (isNameless || isAnonymousNamespace)
            {   // if nameless, enumSig will look like this:  "enum (unnamed enum : Red) { Red = 0, Green = 1, Blue = 2 };"
                std::string enumSig = SerializeDecl(contextItems, enumType->getDecl());
                enumSig = enumSig.substr(0, enumSig.size()-2); // strip off ";\n"
                if (isAnonymousNamespace && isNameless)
                {   // if both, need to insert "(anonymous namespace)::" between the "enum " and "(unnamed enum"
                    enumSig = enumSig.substr(5); // strip off "enum "
                    enumSig = "enum (anonymous namespace)::" + enumSig;
                }
                return "using " + aliasName + " = " + enumSig + "; // typedef " + enumSig + " " + aliasName + ";\n";
            }

            return "using " + aliasName + " = " + resolvedType + "; // typedef " + resolvedType + " " + aliasName + ";\n";
        }
    };
}