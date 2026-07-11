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
    template<auto SerializeDecl, auto SerializeType, auto SerializeAttr> class CXXRecordDeclSerializer
    {
        const ContextItems & contextItems;
        const CXXRecordDecl* cxxRecordDecl;
    public:
        CXXRecordDeclSerializer(const ContextItems& contextItems, const CXXRecordDecl* cxxRecordDecl) : contextItems(contextItems), cxxRecordDecl(cxxRecordDecl) {}

        std::string get_Kind()        const { return cxxRecordDecl->getKindName().str() + " "; }
        std::string get_Name()        const { return cxxRecordDecl->isAnonymousStructOrUnion() ? "" : cxxRecordDecl->getQualifiedNameAsString(); }
        std::string get_SizeComment() const { return cxxRecordDecl->isCompleteDefinition() && !cxxRecordDecl->isDependentType() ? " // sizeof=" + std::to_string(contextItems.context.getASTRecordLayout(cxxRecordDecl).getSize().getQuantity()) + "\n" : "\n"; }

        std::string Serialize() const
        {
            std::string out;

        // std::string ConstructRecordSignature(const CXXRecordDecl * cxxRecordDecl) {
        //    // template
        //    const clang::ClassTemplateDecl* ctd = cxxRecordDecl->getDescribedClassTemplate();
        //    if (ctd)
        //        out += ConstructTemplateParameterList(ctd->getTemplateParameters());
        //    else if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(cxxRecordDecl))
        //        out += "template<> ";
            
            out += get_Kind(); // struct/class/union keyword

            bool hasFinal = false; // final is treated as an attribute, but it's really a keyword
            for (const Attr* attr : cxxRecordDecl->attrs()) // alignas/[[attributes]]/__declspecs
            {
                std::string a = SerializeAttr(contextItems, attr);
                if (a == "final ")
                    hasFinal = true;
                else
                    out += SerializeAttr(contextItems, attr);
            }

        //    // name (if not nameless)
        //    if (!cxxRecordDecl->isAnonymousStructOrUnion())
        //        out += cxxRecordDecl->getQualifiedNameAsString(); // note: no space until after any <> stuff

            out += get_Name(); // NOTE: no " " until after template stuff

        //    // if it's a template instantiation, add <arg> 
        //    if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(cxxRecordDecl))
        //    {
        //        out += IndentBlock(TemplateArgsToString(CTSD), out.size());
        //        out  = out.substr(0, out.size()-1); // strip off last "\n"
        //    }

            out += " ";

            if (hasFinal) // final is treated as an attribute, but it's really a keyword
                out += "final ";

        //    // base classes
        //    std::string indentToColon(out.size(), ' ');
        //    bool previousWasAnonymous = false;
        //    bool firstBase = true;
        //    for (const clang::CXXBaseSpecifier& base : cxxRecordDecl->bases())
        //    {
        //        if (firstBase) {
        //            firstBase = false;
        //            out += ": ";
        //        } else {
        //            if (previousWasAnonymous) {
        //                previousWasAnonymous = false;
        //                out += "\n" + indentToColon;
        //            }
        //            out += ", ";
        //        }

        //        switch (base.getAccessSpecifier()) {
        //        case clang::AS_public:    out += "public ";    break;
        //        case clang::AS_protected: out += "protected "; break;
        //        case clang::AS_private:   out += "private ";   break;
        //        case clang::AS_none:      out += " ";          break; // depends on context
        //        }
        //        if (base.isVirtual())
        //            out += "virtual ";

        //        // when a base is defined in an anonymous namespace, include the full definition here.
        //        const clang::Type* type = base.getType().getCanonicalType().getTypePtr();
        //        const auto* recordType  = dyn_cast<RecordType>(type);
        //        if (recordType && recordType->getDecl()->isInAnonymousNamespace())
        //        {
        //            previousWasAnonymous = true;
        //            out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), out.size() - (out.rfind('\n') + 1));  // length of last line up to current spot
        //            out  = out.substr(0, out.size()-2); // strip off last ";\n"
        //        }
        //        else
        //            out += base.getType().getAsString(printPolicy);
        //    }
        //    if (firstBase == false)
        //        out += " ";

        //    // final
        //    if (true == cxxRecordDecl->hasAttr<FinalAttr>())
        //        out += "final ";

        //    // sizeof as comment
        //    if (cxxRecordDecl->isCompleteDefinition() && !cxxRecordDecl->isDependentType())
        //        out += "{ // sizeof=" + std::to_string(context->getASTRecordLayout(cxxRecordDecl).getSize().getQuantity()) + "\n";
        //    else
        //        out += "{\n";

            out += "{";
            out += get_SizeComment();


        //    // data-members and methods
        //    for (const clang::Decl* decl : cxxRecordDecl->decls())
        //    {
        //        if (clang::isa<clang::AccessSpecDecl>(decl))
        //        {
        //            const auto* access = clang::dyn_cast<clang::AccessSpecDecl>(decl);
        //            switch (access->getAccess())
        //            {
        //            case AS_public:    out += "public:\n";    break;
        //            case AS_protected: out += "protected\n:"; break;
        //            case AS_private:   out += "private:\n";   break;
        //            default:                                  break;
        //            }
        //            continue;
        //        }
        //        if (const auto* field = clang::dyn_cast<clang::FieldDecl>(decl))
        //        {   // data members

        //            if (field->isAnonymousStructOrUnion())
        //                continue; // nameless unions/struct never have a variable name

        //            out += "   ";

        //            // attributes on data-members
        //            out += ConstructAttributes(decl);

        //            { // when a field is defined in an anonymous namespace, include the full definition here with the field.
        //                IndirectionCvStripper ics(field->getType().getCanonicalType());
        //                const QualType qualType = ics.GetBaseType();
        //                const clang::Type* type = qualType.getTypePtr();

        //                std::string definition;

        //                const auto* recordType  = clang::dyn_cast<clang::RecordType>(type);
        //                if (recordType && recordType->getDecl()->isInAnonymousNamespace())
        //                {
        //                    definition = IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), 3);
        //                    definition = definition.substr(0, definition.size()-2);
        //                }
        //                else if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(type); enumTy && !enumTy->getDecl()->getIdentifier())
        //                    definition = ConstructEnumDefinition(enumTy->getDecl()); // nameless enum
        //                else if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(type); enumTy && enumTy->getDecl()->isInAnonymousNamespace())
        //                    definition = ConstructEnumDefinition(enumTy->getDecl()); // enum defined in anonymous namespace

        //                if (!definition.empty()) {
        //                    out += ics.ConstructPrefix() + ConstructAttributes(field);
        //                    out += definition;
        //                    out += ics.ConstructPointersAndReferences() + ics.ConstructSuffixWithName(field->getNameAsString());
        //                } else {
        //                    // field must be done this way to handle array fields as well.
        //                    std::string fieldStr;
        //                    llvm::raw_string_ostream os(fieldStr);
        //                    field->getType().print(os, printPolicy, field->getNameAsString());
        //                    os.flush();
        //                    out += fieldStr;
        //                }
        //            }

        //            if (field->isBitField())
        //            {
        //                std::string bitWidth;
        //                llvm::raw_string_ostream os(bitWidth);
        //                field->getBitWidth()->printPretty(os, nullptr, printPolicy);
        //                os.flush();
        //                out += " : " + bitWidth;
        //            }

        //            if (field->hasInClassInitializer())
        //                out += AddClassInitializer(field);

        //            out += ";\n";
        //            continue;
        //        }
        //        if (const auto* var = clang::dyn_cast<clang::VarDecl>(decl))
        //        {   // static data members  (VarDecl inside a record == static member)
        //            out += "   ";

        //            // attributes on static data-members
        //            out += ConstructAttributes(decl);

        //            if (var->isConstexpr())
        //                out += "constexpr ";
        //            else if (var->isInline())
        //                out += "inline ";

        //            { // static field: use same type+name print trick as non-static
        //                std::string fieldStr;
        //                llvm::raw_string_ostream os(fieldStr);
        //                var->getType().print(os, printPolicy, var->getNameAsString());
        //                os.flush();
        //                out += "static " + fieldStr;
        //            }

        //            if (var->hasInit())
        //            {
        //                const Expr* expr = var->getInit();
        //                llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), context->getSourceManager(), context->getLangOpts());
        //                std::string      init = text.str();
        //                if ((init.starts_with("{")) || init.starts_with("("))
        //                    out += init;
        //                else
        //                    out += "=" + init;
        //            }
        //            out += ";\n";
        //            continue;
        //        }
        //        if (const auto* method = clang::dyn_cast<clang::CXXMethodDecl>(decl))
        //        {   // methods
        //            if (method->isImplicit())
        //                continue;

        //            out += IndentBlock(ConstructFunctionSignature(method, false), 3, "   ");
        //            continue;
        //        }
        //        if (const auto* funcTemplateDecl = dyn_cast<FunctionTemplateDecl>(decl))
        //        {
        //            out += IndentBlock(ConstructFunctionSignature(funcTemplateDecl->getTemplatedDecl(), false), 3, "   ");
        //            continue;
        //        }

        //        if (const auto* nested = clang::dyn_cast<clang::CXXRecordDecl>(decl))
        //        {
        //            if (nested->isInjectedClassName())
        //                continue;

        //            if (nested->isLambda())
        //                continue;

        //            out += IndentBlock(ConstructRecordSignature(nested), 3, "   ");
        //            continue;
        //        }
        //        if (clang::isa<clang::IndirectFieldDecl>(decl))
        //            continue; // these are Clang's mechanism for exposing the members of an anonymous struct/union as if they were directly accessible in the enclosing class.

        //        if (const auto* enumDecl = clang::dyn_cast<clang::EnumDecl>(decl))
        //        {   // a locally defined enum
        //            out += ConstructEnumDefinition(enumDecl) + ";";
        //            continue;
        //        }

        //        if (const auto* nestedTemplate = clang::dyn_cast<clang::ClassTemplateDecl>(decl))
        //        {
        //            const clang::CXXRecordDecl* templated = nestedTemplate->getTemplatedDecl();
        //            if (!templated->isCompleteDefinition())
        //            {
        //                const clang::TemplateParameterList* params = nestedTemplate->getTemplateParameters();
        //                out += "   " + ConstructTemplateParameterList(params);
        //                out += templated->getKindName().str() + " ";
        //                out += nestedTemplate->getNameAsString() + ";\n";
        //                continue;
        //            }

        //            out += IndentBlock(ConstructRecordSignature(templated), 3, "   ");
        //            continue;
        //        }

        //        if (auto* friendDecl = dyn_cast<FriendDecl>(decl))
        //        {
        //            if (auto* namedFriendDecl = friendDecl->getFriendDecl())
        //            {
        //                if (auto* funcDecl = dyn_cast<FunctionDecl>(namedFriendDecl))
        //                {
        //                    out += IndentBlock(ConstructFunctionSignature(funcDecl, false), 3, "   ");
        //                    continue;
        //                }
        //                if (auto* funcTemplateDecl = dyn_cast<FunctionTemplateDecl>(namedFriendDecl))
        //                {
        //                    out += IndentBlock(ConstructFunctionSignature(funcTemplateDecl->getTemplatedDecl(), false), 3, "   ");
        //                    continue;
        //                }
        //                if (auto* classTemplateDecl = dyn_cast<ClassTemplateDecl>(namedFriendDecl))
        //                {
        //                    const auto* templated = classTemplateDecl->getTemplatedDecl();
        //                    if (!templated->isCompleteDefinition())
        //                    {
        //                        const clang::TemplateParameterList* params = classTemplateDecl->getTemplateParameters();
        //                        out += "   " + ConstructTemplateParameterList(params) + "friend ";
        //                        out += templated->getKindName().str() + " ";
        //                        out += classTemplateDecl->getQualifiedNameAsString() + ";\n";
        //                        continue;
        //                    }

        //                    out += IndentBlock(ConstructRecordSignature(templated), 3, "   ");
        //                    continue;
        //                }
        //            }
        //            if (auto* friendTypeSourceInfo = friendDecl->getFriendType())
        //            {
        //                out += "   friend " + friendTypeSourceInfo->getType().getAsString(printPolicy) + ";\n";
        //                continue;
        //            }
        //        }

        //        if (auto* typedefDecl = dyn_cast<TypedefDecl>(decl))
        //        {
        //            out += IndentBlock(ConstructTypedefSignature(typedefDecl), out.size() - (out.rfind('\n') + 1) + 3); // length of last line up to current spot (+3 for indenting)
        //            continue;
        //        }
        //        if (auto* typeAliasDecl = dyn_cast<TypeAliasDecl>(decl))
        //        {
        //            std::string      aliasName = typeAliasDecl->getNameAsString();
        //            QualType        underlying = typeAliasDecl->getUnderlyingType();
        //            if (const auto* recordType = underlying->getAs<RecordType>())
        //            {
        //                if (recordType->getDecl()->isInAnonymousNamespace())
        //                {
        //                    out += "   using " + aliasName + " = ";
        //                    out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), 12 + aliasName.size());
        //                    continue;
        //                }
        //            }
        //            out += "   using " + aliasName + " = " + underlying.getAsString(printPolicy) + ";\n";
        //            continue;
        //        }
        //        if (const auto* tatDecl = dyn_cast<TypeAliasTemplateDecl>(decl))
        //        {
        //            out += IndentBlock(ConstructTemplateAliasSignature(tatDecl), out.size() - (out.rfind('\n') + 1)); // length of last line up to current spot
        //            continue;
        //        }
        //            
        //        { // unhandled
        //            out += "unhandled clang::Decl ";
        //            out += decl->getDeclKindName();
        //            out += " ";
        //            if (const auto* named = clang::dyn_cast<clang::NamedDecl>(decl))
        //                out += named->getNameAsString();
        //            out += "\n";
        //        }
        //    }

            out += "};";

            return out;
        }
    };
}