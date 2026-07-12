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
            static std::string SerializeFunctionTemplateDecl(const ContextItems& contextItems, const FunctionTemplateDecl* functionTemplateDecl)
            {
                return SerializeFunctionDecl(contextItems, functionTemplateDecl->getTemplatedDecl());
            }
            static std::string SerializeAccessSpecDecl(const ContextItems& contextItems, const AccessSpecDecl* accessDecl)
            {
                switch (accessDecl->getAccess())
                {
                case AS_public:    return "public:\n";
                case AS_protected: return "protected\n:";
                case AS_private:   return "private:\n";
                default:           return "";
                }
            }
            static std::string SerializeFieldDecl(const ContextItems& contextItems, const FieldDecl* fieldDecl)
            {
                if (fieldDecl->isAnonymousStructOrUnion())
                    return ""; // nameless unions/struct never have a variable name

                std::string out;

                // attributes on data-members
                //out += ConstructAttributes(decl);

                //{ // when a field is defined in an anonymous namespace, include the full definition here with the field.
                //    IndirectionCvStripper ics(field->getType().getCanonicalType());
                //    const QualType qualType = ics.GetBaseType();
                //    const clang::Type* type = qualType.getTypePtr();

                //    std::string definition;

                //    const auto* recordType  = clang::dyn_cast<clang::RecordType>(type);
                //    if (recordType && recordType->getDecl()->isInAnonymousNamespace())
                //    {
                //        definition = IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), 3);
                //        definition = definition.substr(0, definition.size()-2);
                //    }
                //    else if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(type); enumTy && !enumTy->getDecl()->getIdentifier())
                //        definition = ConstructEnumDefinition(enumTy->getDecl()); // nameless enum
                //    else if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(type); enumTy && enumTy->getDecl()->isInAnonymousNamespace())
                //        definition = ConstructEnumDefinition(enumTy->getDecl()); // enum defined in anonymous namespace

                //    if (!definition.empty()) {
                //        out += ics.ConstructPrefix() + ConstructAttributes(field);
                //        out += definition;
                //        out += ics.ConstructPointersAndReferences() + ics.ConstructSuffixWithName(field->getNameAsString());
                {
                    // field must be done this way to handle array fields as well.
                    std::string fieldStr;
                    llvm::raw_string_ostream os(fieldStr);
                    fieldDecl->getType().print(os, contextItems.printPolicy, fieldDecl->getNameAsString());
                    os.flush();

                    if (auto* recordDecl = fieldDecl->getType()->getAsRecordDecl();
                              recordDecl && (recordDecl->getIdentifier() == nullptr) && (recordDecl->getTypedefNameForAnonDecl() == nullptr))
                    {
                        fieldStr = OdrCop3::MakeUnnamedAndAnonymousConsistent(fieldStr);

                        std::string qualifier;
                        if (auto* parentDecl = llvm::dyn_cast<clang::NamedDecl>(recordDecl->getDeclContext()))
                            qualifier = parentDecl->getQualifiedNameAsString() + "::";

                        std::string anonymous = "(anonymous type at ";
                        auto pos = fieldStr.find(anonymous);
                        if (pos != std::string::npos)
                            fieldStr.insert(pos, qualifier);
                    }

                    out += fieldStr;
                }

                if (fieldDecl->isBitField())
                {
                    std::string bitWidth;
                    llvm::raw_string_ostream os(bitWidth);
                    fieldDecl->getBitWidth()->printPretty(os, nullptr, contextItems.printPolicy);
                    os.flush();
                    out += ":" + bitWidth;
                }

                if (fieldDecl->hasInClassInitializer())
                {
                    const Expr* expr = fieldDecl->getInClassInitializer();
                    //const auto* declRef   = llvm::dyn_cast<clang::DeclRefExpr>(expr->IgnoreParenImpCasts());
                    //const auto* enumConst = declRef ? llvm::dyn_cast<clang::EnumConstantDecl>(declRef->getDecl()) : nullptr;
                    //if (enumConst)
                    //{
                    //    const auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(enumConst->getDeclContext());
                    //    out += "=" + ConstructEnumName(enumDecl, enumConst);
                    //}
                    //else
                    {
                        llvm::StringRef text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts());
                        std::string     init = text.str();
                        if ((init.starts_with("{")) || init.starts_with("("))
                            out += init;
                        else
                            out += "=" + init;
                    }
                }

                out += ";\n";
                return out;
            }
            static std::string SerializeVarDecl(const ContextItems& contextItems, const VarDecl* varDecl)
            {
                std::string out;

                for (const auto* attr : varDecl->attrs())
                    out += SerializeAttr(contextItems, attr); // attributes on static data-members

                if (varDecl->isConstexpr())
                    out += "constexpr ";
                else if (varDecl->isInline()) // constexpr is inline; don't print both
                    out += "inline ";

                { // static field: use same type+name print trick as non-static
                    std::string fieldStr;
                    llvm::raw_string_ostream os(fieldStr);
                    varDecl->getType().print(os, contextItems.printPolicy, varDecl->getNameAsString());
                    os.flush();
                    out += "static " + fieldStr;
                }

                if (varDecl->hasInit())
                {
                    const Expr* expr = varDecl->getInit();
                    llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts());
                    std::string      init = text.str();
                    if ((init.starts_with("{")) || init.starts_with("("))
                        out += init;
                    else
                        out += "=" + init;
                }
                out += ";\n";
                return out;
            }
            static std::string SerializeParameterDecl(const ContextItems& contextItems, const ParmVarDecl* paramDecl)
            {
                std::string out;

                {   // leading attributes
                    if (TypeSourceInfo* typeSourceInfo = paramDecl->getTypeSourceInfo())
                    {
                        SourceLocation typeLoc = typeSourceInfo->getTypeLoc().getBeginLoc();
                        for (const Attr* attr : paramDecl->attrs())
                        {
                            if (attr->getLocation() > typeLoc)
                                continue; // this is a trailing attribute
                            out += SerializeAttr(contextItems, attr);
                        }
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

                {   // trailing attributes
                    if (TypeSourceInfo* typeSourceInfo = paramDecl->getTypeSourceInfo())
                    {
                        SourceLocation typeLoc = typeSourceInfo->getTypeLoc().getBeginLoc();
                        for (const Attr* attr : paramDecl->attrs())
                        {
                            if (attr->getLocation() < typeLoc)
                                continue; // leading attribute
                            out += " " + SerializeAttr(contextItems, attr);
                            out  = out.substr(0, out.size()-1); // strip off last " "
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
            case clang::Decl::Kind::CXXMethod:        // is a subclass of FunctionDecl
            case clang::Decl::Kind::CXXConstructor:   // so is this
            case clang::Decl::Kind::CXXConversion:    // and this
            case clang::Decl::Kind::Function:         if (const FunctionDecl* functionDecl = dyn_cast<FunctionDecl        >(decl)) return DeclSerializer::SerializeFunctionDecl  (contextItems, functionDecl); break;
            case clang::Decl::Kind::ParmVar:          if (const ParmVarDecl *          pvd = dyn_cast<ParmVarDecl         >(decl)) return DeclSerializer::SerializeParameterDecl (contextItems, pvd);          break;
            case clang::Decl::Kind::CXXRecord:        if (const CXXRecordDecl*         cxx = dyn_cast<CXXRecordDecl       >(decl)) return DeclSerializer::SerializeCXXRecordDecl (contextItems, cxx);          break;
            case clang::Decl::Kind::Field:            if (const FieldDecl *      fieldDecl = dyn_cast<FieldDecl           >(decl)) return DeclSerializer::SerializeFieldDecl     (contextItems, fieldDecl);    break;
            case clang::Decl::Kind::FunctionTemplate: if (const FunctionTemplateDecl * ftd = dyn_cast<FunctionTemplateDecl>(decl)) return DeclSerializer::SerializeFunctionTemplateDecl(contextItems, ftd);    break;
            case clang::Decl::Kind::AccessSpec:       if (const AccessSpecDecl* accessDecl = dyn_cast<AccessSpecDecl      >(decl)) return DeclSerializer::SerializeAccessSpecDecl(contextItems, accessDecl);   break;
            case clang::Decl::Kind::Var:              if (const VarDecl *          varDecl = dyn_cast<VarDecl             >(decl)) return DeclSerializer::SerializeVarDecl       (contextItems, varDecl);      break;
            default: break;
            }
            throw OdrCop3::UnhandledException(std::string("unhandled decl::getKind: ") + enum_name(decl->getKind()));
        }
    }
}