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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class TypeAliasDeclSerializer
    {
        const ContextItems & contextItems;
        const TypeAliasDecl* typeAliasDecl;
    public:
        TypeAliasDeclSerializer(const ContextItems& contextItems, const TypeAliasDecl* typeAliasDecl) : contextItems(contextItems), typeAliasDecl(typeAliasDecl) {}

        std::string Serialize() const
        {
            // typeAliasDecl->getUnderlyingType().dump();

            std::string aliasName    = typeAliasDecl->getQualifiedNameAsString();
            QualType    underlying   = typeAliasDecl->getUnderlyingType().getCanonicalType();
            std::string resolvedType = underlying.getAsString(contextItems.printPolicy);

            // if it's a template type, we don't want "the "type-parameter-0-0"; we want "T"
            if (const auto * templateTypeParmType = typeAliasDecl->getUnderlyingType()->getAs<clang::TemplateTypeParmType>())
            if (const clang::TemplateTypeParmDecl * templateTypeParm = templateTypeParmType->getDecl())
            {
                resolvedType = templateTypeParm->getNameAsString();
                return "using " + aliasName + " = " + resolvedType + "; // typedef " + resolvedType + " " + aliasName + ";\n";
            }

            // if it's a template template type, we don't want "the "type-parameter-0-0"; we want "T"
            if (const auto* dependentNameType = typeAliasDecl->getUnderlyingType()->getAs<clang::DependentNameType>())
            {
                std::string result;

                llvm::raw_string_ostream os(result);
                dependentNameType->getQualifier().print(os, contextItems.printPolicy);
                os.flush();

                if (const clang::IdentifierInfo* identifier = dependentNameType->getIdentifier())
                    result += identifier->getName();

                resolvedType = result;
                return "using " + aliasName + " = " + resolvedType + "; // typedef " + resolvedType + " " + aliasName + ";\n";
            }

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
                    fqtd  = TrimRightIf(fqtd, ";\n");
                    fqtd += "; // typedef " + resolvedType + " " + aliasName + ";\n";
                    return fqtd;
                }

                // anonymous-namespace function (or other ValueDecl) used as a non-type template argument
                const ClassTemplateSpecializationDecl* specDecl = dyn_cast<ClassTemplateSpecializationDecl>(recordType->getDecl());
                if (specDecl != nullptr)
                {
                    std::string fqtd2 = "using " + aliasName + " = " + specDecl->getSpecializedTemplate()->getNameAsString() + "<";

                    // lastColumn and column are absolute
                    int lastColumn = 0;

                    std::string argList;

                    bool anyInlined = false;
                    const TemplateArgumentList& args = specDecl->getTemplateArgs();
                    for (unsigned i=0; i<args.size(); ++i)
                    {
                        const TemplateArgument& arg = args.get(i);
                        bool isAnonNsFunc = false;
                        if (i != 0)
                            argList += ", ";

                        const FunctionDecl* funcDecl = nullptr;
                        if (arg.getKind() == TemplateArgument::Declaration)
                            funcDecl = dyn_cast<FunctionDecl>(arg.getAsDecl());
                        if (funcDecl != nullptr) {
                            const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(funcDecl->getDeclContext());
                            if (nsDecl != nullptr && nsDecl->isAnonymousNamespace())
                                isAnonNsFunc = true;
                        }

                        if (isAnonNsFunc) {
                            argList += "&(";

                            int column = static_cast<int>(argList.size() - (argList.rfind('\n') + 1));
                            if (lastColumn == 0)
                                column += static_cast<int>(fqtd2.size()); // only first time

                            argList += IndentBlock(SerializeDecl(contextItems, funcDecl), column); // -lastColumn);
                            argList  = TrimRightIf(argList, "\n");
                            argList += ")";

                            anyInlined = true;
                            lastColumn = column;
                        } else {
                            llvm::raw_string_ostream stream(argList);
                            arg.print(contextItems.printPolicy, stream, /*IncludeType=*/false);
                        }
                    }

                    if (anyInlined)
                    {
                        size_t col1 = fqtd2.size() - (fqtd2.rfind('\n')+1);// get length of last line of fqtd2

                        std::string whatWillBeReused = argList + ">";
                        fqtd2 += whatWillBeReused + "; // typedef " + specDecl->getSpecializedTemplate()->getNameAsString() + "<";

                        size_t col2 = fqtd2.size() - (fqtd2.rfind('\n')+1); // get length of last line of fqtd2
                        fqtd2 += IndentBlock(whatWillBeReused, col2-col1);
                        fqtd2  = TrimRightIf(fqtd2, "\n");
                        fqtd2 += " " + aliasName + ";\n";
                        return fqtd2;
                    }
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
                enumSig = TrimRightIf(enumSig, ";\n");
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