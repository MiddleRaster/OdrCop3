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

#include "SerializationUtils.h"

namespace OdrCop3
{

    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class FieldDeclSerializer
    {
        const ContextItems & contextItems;
        const FieldDecl     * fieldDecl;
    public:
        FieldDeclSerializer(const ContextItems& contextItems, const FieldDecl* fieldDecl) : contextItems(contextItems), fieldDecl(fieldDecl) {}

        std::string Serialize() const
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
    };
}