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
    public:
        FieldDeclSerializer(const ContextItems& contextItems, const FieldDecl* fieldDecl) : contextItems(contextItems), fieldDecl(fieldDecl) {}

    private:
        static bool ContainsAnonymousType(clang::QualType qualType)
        {
            qualType = qualType.getCanonicalType();

            if (const auto* recordType = qualType->getAs<clang::RecordType>())
                return IsDefinedInAnonymousNamespace(static_cast<const Decl*>(recordType->getDecl()));

            if (const auto* enumType = qualType->getAs<clang::EnumType>())
                return IsDefinedInAnonymousNamespace(static_cast<const Decl*>(enumType->getDecl()));

            if (qualType->isPointerType() || qualType->isReferenceType())
                return ContainsAnonymousType(qualType->getPointeeType());

            if (const auto* memberPointerType = qualType->getAs<clang::MemberPointerType>())
            {
                if (const clang::CXXRecordDecl* classDecl = memberPointerType->getMostRecentCXXRecordDecl())
                    if (true == IsDefinedInAnonymousNamespace(static_cast<const Decl*>(classDecl)))
                        return true;
                return ContainsAnonymousType(memberPointerType->getPointeeType());
            }

            if (qualType->isArrayType())
                return ContainsAnonymousType(clang::QualType(qualType->getArrayElementTypeNoTypeQual(), 0));

            if (const auto* fnProtoType = qualType->getAs<clang::FunctionProtoType>())
            {
                if (true == ContainsAnonymousType(fnProtoType->getReturnType()))
                    return true;
                for (clang::QualType paramType : fnProtoType->getParamTypes())
                    if (true == ContainsAnonymousType(paramType))
                        return true;
            }
            return false;
        }
        static bool IsPointerOrReferenceType(clang::QualType qualType)
        {
            return qualType->isPointerType()         ||
                   qualType->isReferenceType()       ||
                   qualType->isMemberPointerType()   ||
                   qualType->isFunctionPointerType() ||
                   qualType->isFunctionReferenceType();
        }
        static const clang::FunctionProtoType* GetInnerFunctionProtoType(clang::QualType qualType)
        {
            if (const auto* fnProtoType = qualType->getAs<clang::FunctionProtoType>())
                return fnProtoType;
            if (qualType->isPointerType() || qualType->isReferenceType())
                return GetInnerFunctionProtoType(qualType->getPointeeType());
            if (const auto* memberPointerType = qualType->getAs<clang::MemberPointerType>())
                return GetInnerFunctionProtoType(memberPointerType->getPointeeType());
            return nullptr;
        }
        const clang::TemplateTypeParmType* IsTemplateArrayType() const
        {
            if (const auto* arrayType = llvm::dyn_cast<clang::DependentSizedArrayType>(fieldDecl->getType().getTypePtr()))
                if (const auto* ttp = arrayType->getElementType()->getAs<clang::TemplateTypeParmType>())
                    return ttp;
            return nullptr;
        }

    public:
        std::string get_PrefixBeforeUnqualifiedPointeeType() const
        {
            std::string str;

            clang::QualType qualType = fieldDecl->getType();
            for (;;)
            {
                if (IsPointerOrReferenceType(qualType))
                    qualType = qualType->getPointeeType();
                else 
                if (qualType->isArrayType())
                    qualType = clang::QualType(qualType->getArrayElementTypeNoTypeQual(), 0);
                else
                    break;
            }
            if (qualType.isConstQualified())    str += "const ";
            if (qualType.isVolatileQualified()) str += "volatile ";
            return str.size() == 0 ? "" : str;
        }
        std::string get_SuffixAfterUnqualifiedPointeeType() const
        {
            std::string str;

            struct Level
            {
                bool isPointer;
                bool isReference;
                bool isConst;
                bool isVolatile;
            };
            std::vector<Level> levels;

            clang::QualType qualType = fieldDecl->getType();
            for (;;)
            {
                if (qualType->isMemberPointerType())
                {
                    const auto* mpt = qualType->getAs<clang::MemberPointerType>();
                    if (const clang::CXXRecordDecl* cxxRecordDecl = mpt->getMostRecentCXXRecordDecl())
                    {
                        str += " ";
                        if (IsDefinedInAnonymousNamespace(static_cast<const Decl*>(cxxRecordDecl)))
                        {
                            str += IndentBlock(SerializeDecl(contextItems, cxxRecordDecl), 0);
                            str  = str.substr(0, str.size()-2);
                        }
                        else
                        {
                            std::string className;
                            llvm::raw_string_ostream os(className);
                            cxxRecordDecl->getCanonicalDecl()->printQualifiedName(os);
                            str += className;
                        }
                        str += "::*";
                    }
                    qualType = mpt->getPointeeType();
                } else
                if (qualType->isPointerType() || qualType->isReferenceType())
                {
                    levels.push_back({qualType->isPointerType(), qualType->isReferenceType(), qualType.isConstQualified(), qualType.isVolatileQualified()});
                    qualType = qualType->getPointeeType();
                } else
                if (qualType->isArrayType())
                    qualType = clang::QualType(qualType->getArrayElementTypeNoTypeQual(), 0);
                else
                    break;
            }
            for (auto it=levels.rbegin(); it != levels.rend(); ++it)
            {
                if (it->isPointer)   str += " *";
                if (it->isReference) str += " &";
                if (it->isConst)     str += " const";
                if (it->isVolatile)  str += " volatile";
            }
            return str.size() == 0 ? " " : str;
        }
        std::string get_Name() const { return fieldDecl->getNameAsString(); }
        std::string get_ArraySuffix() const
        {
            std::string suffix;

            QualType qt = fieldDecl->getType();
            while (IsPointerOrReferenceType(qt))
                qt = qt->getPointeeType();

            // Now peel array layers
            while (qt->isArrayType())
            {
                if (const auto* CAT = llvm::dyn_cast<clang::ConstantArrayType>(qt))
                {
                    llvm::APInt size = CAT->getSize();
                    suffix += "[" + std::to_string(size.getZExtValue()) + "]";
                    qt = clang::QualType(CAT->getElementType().getTypePtr(), 0);
                }
                else if (const auto* IAT = llvm::dyn_cast<clang::IncompleteArrayType>(qt))
                {
                    suffix += "[]";
                    qt = clang::QualType(IAT->getElementType().getTypePtr(), 0);
                }
                else if (const auto* VAT = llvm::dyn_cast<clang::VariableArrayType>(qt))
                {
                    suffix += "[/* variable length */]";
                    qt = clang::QualType(VAT->getElementType().getTypePtr(), 0);
                }
                else if (const auto* DSAT = llvm::dyn_cast<clang::DependentSizedArrayType>(qt))
                {
                    suffix += "[/* dependent */]";
                    qt = clang::QualType(DSAT->getElementType().getTypePtr(), 0);
                }
                else
                    break;
            }
            return suffix;
        }
        const clang::FunctionProtoType* get_PointerToFunctionWithAnonymousReturnOrArgs() const
        {
            std::string diagnostic = fieldDecl->getType()->getTypeClassName();

            QualType qualType = fieldDecl->getType();
            if (const clang::FunctionProtoType* fnProtoType = GetInnerFunctionProtoType(qualType))
                if (true == ContainsAnonymousType(qualType))
                    return fnProtoType;

            return nullptr;
        }

        std::string Serialize() const
        {
            if (fieldDecl->isAnonymousStructOrUnion())
                return ""; // nameless unions/struct never have a variable name

            std::string out;

            // attributes on data-members
            for (const Attr* attr : fieldDecl->attrs())
                out += SerializeAttr(contextItems, attr);

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

            // is the field actually a pointer-to-function or pointer-to-member-functin?
            if (const clang::FunctionProtoType* fnProtoType = get_PointerToFunctionWithAnonymousReturnOrArgs())
            {
                int indentation = 0;

                // insert function pointer variable name into function prototype
                std::string fpStr = get_SuffixAfterUnqualifiedPointeeType();
                if (fpStr.starts_with(" "))
                    fpStr = fpStr.substr(1); // skip space
                fpStr += get_Name() + get_ArraySuffix();

                if (fpStr.find("\n") != std::string::npos)
                {   // if multiline, do this twice: first to figure out what the indentation needs to be; then again with the right indentation
                    ContextItems ci2(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, " (?????)");
                    std::string placeHolder = SerializeType(ci2, clang::QualType(fnProtoType, 0));
                    indentation = static_cast<int>(placeHolder.find("?????"));
                }

                ContextItems ci(&contextItems.context, contextItems.printPolicy, contextItems.TU, contextItems.recursingDecls, " (" + fpStr + ")");
                out += IndentBlock(SerializeType(ci, clang::QualType(fnProtoType, 0)), indentation);
                if (out.ends_with('\n'))
                    out = out.substr(0, out.size() - 1); // remove '\n'
            }
            else if (IsDefinedInAnonymousNamespace(fieldDecl))
            {
                out += get_PrefixBeforeUnqualifiedPointeeType();

                out += IndentBlock(SerializeDecl(contextItems, StripPointersAndReferences(fieldDecl)), out.size() - (out.rfind('\n')+1));
                if (out.ends_with('\n'))
                    out = out.substr(0, out.size()-1); // remove '\n'
                if (out.ends_with(';'))
                    out = out.substr(0, out.size()-1); // remove ';'

                out += get_SuffixAfterUnqualifiedPointeeType();
                out += get_Name();
                out += get_ArraySuffix();
            }
            else if (const clang::TemplateTypeParmType* ttp = IsTemplateArrayType())
            {   // e.g., T field[N];
                const clang::TemplateTypeParmDecl* paramDecl = ttp->getDecl();
                out += paramDecl->getNameAsString();   // "T"
                out += " " + get_Name();

                const auto   * arrayType = llvm::dyn_cast<clang::DependentSizedArrayType>(fieldDecl->getType().getTypePtr());
                const clang::Expr * expr = arrayType->getSizeExpr();
                llvm::StringRef     text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), contextItems.context.getSourceManager(), contextItems.context.getLangOpts());

                out += "[";
                out += text.str();
                out += "]";
            }
            else
            {   // field must be done this way to handle array fields as well.
                std::string fieldStr;
                clang::QualType qt = fieldDecl->getType().getCanonicalType(); // see through typedefs and using aliases
                llvm::raw_string_ostream os(fieldStr);
                qt.print(os, contextItems.printPolicy, (qt->isMemberDataPointerType() ? " " : "") + fieldDecl->getName()); // adding a space before the field in the pointer-to-member-data case only
                os.flush();

                // is it an anonymous struct/class/union/enum? If so, normalize text
                if (auto* tagDecl = fieldDecl->getType()->getAsTagDecl();
                          tagDecl && (tagDecl->getIdentifier() == nullptr) && (tagDecl->getTypedefNameForAnonDecl() == nullptr))
                {
                    fieldStr = tagDecl->getKindName().str() + " " + OdrCop3::MakeUnnamedAndAnonymousConsistent(fieldStr);
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