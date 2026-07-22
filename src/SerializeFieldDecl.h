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
        const ContextItems& contextItems;
        const FieldDecl   * fieldDecl;
        
        bool IsTemplateParamType() const
        {
            const clang::Type* ty = fieldDecl->getType().getTypePtr();
            while (true)
            {
                     if (auto*   ptr = llvm::dyn_cast<clang::PointerType>(ty))   ty = ptr->getPointeeType().getTypePtr();
                else if (auto*   ref = llvm::dyn_cast<clang::ReferenceType>(ty)) ty = ref->getPointeeType().getTypePtr();
                else if (auto*   arr = llvm::dyn_cast<clang::ArrayType>(ty))     ty = arr->getElementType().getTypePtr();
                else if (auto* paren = llvm::dyn_cast<clang::ParenType>(ty))     ty = paren->getInnerType().getTypePtr();
                else break;
            }
            return llvm::isa<clang::TemplateTypeParmType>(ty);
        }
        std::string get_Name() const { return fieldDecl->getNameAsString(); }

    public:
        FieldDeclSerializer(const ContextItems& contextItems, const FieldDecl* fieldDecl) : contextItems(contextItems), fieldDecl(fieldDecl) {}

        std::string Serialize() const
        {
            if (fieldDecl->isAnonymousStructOrUnion())
                return ""; // nameless unions/struct never have a variable name

            std::string out;

            // attributes on data-members
            for (const Attr* attr : fieldDecl->attrs())
                out += SerializeAttr(contextItems, attr);

            if (NeedsManualSerialization(contextItems, fieldDecl->getType()) || IsTemplateParamType())
            {
                ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, get_Name()); // let appropriate type serializer put in the (*blah) part
                out += IndentBlock(SerializeType(ci2, fieldDecl->getType()), out.size() - (out.rfind('\n') + 1));
                out  = TrimRightIf(out, ";");
            }
            else
            {   // field must be done this way to handle array fields as well.
                std::string fieldStr;
                clang::QualType qt = fieldDecl->getType().getCanonicalType(); // see through typedefs and using aliases
                llvm::raw_string_ostream os(fieldStr);
                qt.print(os, contextItems.printPolicy, (qt->isMemberDataPointerType() ? " " : "") + get_Name()); // adding a space before the field in the pointer-to-member-data case only
                os.flush();

                // is it an anonymous struct/class/union/enum? If so...
                if (auto* tagDecl = fieldDecl->getType()->getAsTagDecl();
                          tagDecl && (tagDecl->getIdentifier() == nullptr) && (tagDecl->getTypedefNameForAnonDecl() == nullptr))
                {   //     ... add "struct"/"class"/"union"   and        ... normalize text
                    fieldStr = tagDecl->getKindName().str() + " " + MakeUnnamedAndAnonymousConsistent(fieldStr);
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