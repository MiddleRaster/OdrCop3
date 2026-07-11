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

#include <vector>
#include <exception>

#include "SerializationUtils.h"
#include "SerializeFunctionDecl.h"
#include "SerializeCXXRecordDecl.h"
#include "magic_enum.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeType, auto SerializeAttr>
        struct Decl
        {
            static std::string SerializeFunctionDecl(const ContextItems& contextItems, const FunctionDecl* funcDecl)
            {
                return FunctionDeclSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, funcDecl).Serialize();
            }
            static std::string SerializeParameterDecl(const ContextItems& contextItems, const ParmVarDecl* paramDecl)
            {
                std::string out;

                // std::string get_LeadingAttributes() const
                {
                    SourceLocation typeLoc = paramDecl->getTypeSourceInfo()->getTypeLoc().getBeginLoc();
                    for (const Attr* attr : paramDecl->attrs())
                    {
                        if (attr->getLocation() > typeLoc)
                            continue; // trailing attribute
                        out += SerializeAttr(contextItems, attr);
                    }
                }

                // type
                out += paramDecl->getType().getAsString(contextItems.printPolicy);

                // name if any
                if (paramDecl->getIdentifier())
                    out += " " + paramDecl->getName().str(); // name

                // default value if any
                if (paramDecl->hasDefaultArg())
                {   // Default argument, if any
                    std::string s;
                    llvm::raw_string_ostream os(s);
                    paramDecl->getDefaultArg()->printPretty(os, nullptr, contextItems.printPolicy);
                    os.flush();
                    out += " = " + s;
                }

                // std::string get_TrailingAttributes() const
                {
                    SourceLocation typeLoc = paramDecl->getTypeSourceInfo()->getTypeLoc().getBeginLoc();
                    for (const Attr* attr : paramDecl->attrs())
                    {
                        if (attr->getLocation() < typeLoc)
                            continue; // leading attribute
                        out += " " + SerializeAttr(contextItems, attr);
                        out  = out.substr(0, out.size()-1); // strip off last " "
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
            static std::string SerializeCXXRecordDecl(const ContextItems& contextItems, const CXXRecordDecl* cxxRecordDecl)
            {
                return CXXRecordDeclSerializer<SerializeDecl, SerializeType, SerializeAttr>(contextItems, cxxRecordDecl).Serialize();
            }
        };

        template<auto SerializeType, auto SerializeAttr>
        static std::string Decls(const ContextItems& contextItems, const clang::Decl* decl)
        {
            if (!decl)
                throw std::invalid_argument("decl argument was null");

            using DeclSerializer = Serialize::Decl<&Decls<SerializeType, SerializeAttr>, SerializeType, SerializeAttr>;
            switch(decl->getKind())
            {
            case clang::Decl::Kind::CXXMethod:      // is a subclass of FunctionDecl
            case clang::Decl::Kind::CXXConstructor: // so is this
            case clang::Decl::Kind::CXXConversion:  // and this
            case clang::Decl::Kind::Function:         if (const FunctionDecl* functionDecl = dyn_cast<FunctionDecl        >(decl)) return DeclSerializer::SerializeFunctionDecl (contextItems, functionDecl); break;
            case clang::Decl::Kind::ParmVar:          if (const ParmVarDecl *          pvd = dyn_cast<ParmVarDecl         >(decl)) return DeclSerializer::SerializeParameterDecl(contextItems, pvd);          break;
            case clang::Decl::Kind::CXXRecord:        if (const CXXRecordDecl*         cxx = dyn_cast<CXXRecordDecl       >(decl)) return DeclSerializer::SerializeCXXRecordDecl(contextItems, cxx);          break;
            default: break;
            }
            throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));
        }
    }
}