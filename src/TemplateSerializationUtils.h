#pragma once

#include "SerializationUtils.h"

namespace OdrCop3
{
    template<typename T> requires std::is_same_v<T,ClassTemplateSpecializationDecl       > ||
                                  std::is_same_v<T,ClassTemplatePartialSpecializationDecl> ||
                                  std::is_same_v<T,VarTemplateSpecializationDecl         > ||
                                  std::is_same_v<T,VarTemplatePartialSpecializationDecl  >
    inline std::string TemplateArgsToString(const ContextItems& contextItems, const T* ctd)
    {
        const ASTTemplateArgumentListInfo * ArgsAsWritten = ctd->getTemplateArgsAsWritten();
        llvm::ArrayRef<TemplateArgumentLoc> ArgsRef(ArgsAsWritten->getTemplateArgs(), ArgsAsWritten->NumTemplateArgs);

        std::string              out;
        llvm::raw_string_ostream os(out);
        clang::printTemplateArgumentList(os, ArgsRef, contextItems.printPolicy, nullptr);
        os.flush();

        return out;
    }

    template <auto SerializeDecl, auto SerializeType, auto SerializeAttr>
    std::string SerializeExpr(const ContextItems& contextItems, const clang::Expr* expr)
    {
        if (!expr)
            return {};

        if (!NeedsManualSerialization(contextItems, expr))
        {
            std::string exprStr;
            llvm::raw_string_ostream os(exprStr);
            expr->printPretty(os, nullptr, contextItems.printPolicy);
            return exprStr;
        }
        // else do it manually

        if (const auto* declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(expr))
            return declRefExpr->getDecl()->getQualifiedNameAsString(); // this does NOT work:  return SerializeDecl(contextItems, declRefExpr->getDecl());

        if (const auto* unaryOperator = llvm::dyn_cast<clang::UnaryOperator>(expr))
        {
            std::string out;
            switch (unaryOperator->getOpcode())
            {
            case clang::UO_AddrOf: out += "&"; break;
            case clang::UO_Deref:  out += "*"; break;
            case clang::UO_Minus:  out += "-"; break;
            case clang::UO_Plus:   out += "+"; break;
            case clang::UO_Not:    out += "!"; break;
            case clang::UO_LNot:   out += "~"; break;
            default:                           break;
            }
            out += SerializeExpr<SerializeDecl, SerializeType, SerializeAttr>(contextItems, unaryOperator->getSubExpr());
            return out;
        }

        if (const auto* integerLiteral = llvm::dyn_cast<clang::IntegerLiteral>(expr))
        {
            llvm::SmallString<32> str;
            integerLiteral->getValue().toString(str, 10, true);
            return std::string(str);
        }

        if (const auto* boolLiteral = llvm::dyn_cast<clang::CXXBoolLiteralExpr>(expr))
            return boolLiteral->getValue() ? "true" : "false";

        if (const auto* parenExpr = llvm::dyn_cast<clang::ParenExpr>(expr))
            return "(" + SerializeExpr<SerializeDecl, SerializeType, SerializeAttr>(contextItems, parenExpr->getSubExpr()) + ")";

        {
            std::string exprStr;
            llvm::raw_string_ostream os(exprStr);
            expr->printPretty(os, nullptr, contextItems.printPolicy);
            return exprStr;
        }
    }

    template <auto SerializeDecl, auto SerializeType, auto SerializeAttr>
    inline std::string ConstructTemplateParameterList(const ContextItems& contextItems, const clang::TemplateParameterList* params)
    {
        auto AddParameterPackAndNameAndDefaultArgument = [](const ContextItems& contextItems, const auto* tp) -> std::string
                                                         {
                                                             std::string out;
                                                             if (tp->isParameterPack())
                                                                 out += "...";
                                                             if (!tp->getName().empty())
                                                                 out += " " + tp->getName().str();
                                                             if (tp->hasDefaultArgument())
                                                             {
                                                                 std::string defaultStr;
                                                                 llvm::raw_string_ostream defaultStream(defaultStr);
                                                                 tp->getDefaultArgument().getArgument().getAsExpr()->printPretty(defaultStream, nullptr, contextItems.printPolicy);
                                                                 out += "=" + defaultStr;
                                                             }
                                                             return out;
                                                         };

        std::string out = "template<";

        bool first = true;
        for (const clang::NamedDecl* param : *params)
        {
            if (first)
                first = false;
            else
                out += ", ";

            if (const auto* ttp = clang::dyn_cast<clang::TemplateTypeParmDecl>(param))
            {
                out += ttp->wasDeclaredWithTypename() ? "typename" : "class";
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, ttp);
                continue;
            }
            if (const auto* nttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
            {   // NTTP where type is a struct (new to C++20)
                QualType nttpQT = nttp->getType().getCanonicalType();
                if (NeedsManualSerialization(contextItems, nttpQT))
                {
                    out += IndentBlock(SerializeType(contextItems, nttpQT), LengthOfLastLine(out));
                    out  = TrimRightIf(out, " ");
                }
                else
                {
                    std::string declStr;
                    llvm::raw_string_ostream declStream(declStr);
                    nttp->getType().print(declStream, contextItems.printPolicy);
                    out += declStr;
                }
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, nttp);
                continue;
            }
            if (const auto* ttp2 = clang::dyn_cast<clang::TemplateTemplateParmDecl>(param))
            {
                std::string nested = ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeAttr>(contextItems, ttp2->getTemplateParameters());
                out += "template " + nested.substr(std::string("template").size()); // tweak the spacing a tiny bit
                out += "class";
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, ttp2);
                continue;
            }
        }
        out += "> ";

        if (const clang::Expr* requiresClause = params->getRequiresClause())
        {
            out += "requires ";
            if (const auto* conceptExpr = dyn_cast<clang::ConceptSpecializationExpr>(requiresClause))
            {
                out += conceptExpr->getNamedConcept()->getNameAsString();
                out += "<";

                for (const TemplateArgumentLoc& argLoc : conceptExpr->getTemplateArgsAsWritten()->arguments())
                {
                    const TemplateArgument& arg = argLoc.getArgument();
                    switch (arg.getKind())
                    {
                    case clang::TemplateArgument::Type       : out += TrimRightIf(IndentBlock(SerializeType(contextItems, arg.getAsType()), LengthOfLastLine(out)), ";"); break;
                    case clang::TemplateArgument::Expression : out += SerializeExpr<SerializeDecl, SerializeType, SerializeAttr>(contextItems, arg.getAsExpr()); break;
                    case clang::TemplateArgument::Declaration:
                    {
                        if (const clang::Expr* expr = arg.getAsExpr())
                            out += SerializeExpr<SerializeDecl, SerializeType, SerializeAttr>(contextItems, expr);
                        else
                            out += arg.getAsDecl()->getQualifiedNameAsString();
                        break;
                    }
                    case clang::TemplateArgument::Template: out += arg.getAsTemplate().getAsTemplateDecl()->getNameAsString(); break;
                    case clang::TemplateArgument::Integral:
                    {
                        llvm::SmallString<32> str;
                        arg.getAsIntegral().toString(str, 10);
                        out += std::string(str);
                        break;
                    }
                    default:
                    {
                        std::string argStr;
                        llvm::raw_string_ostream os(argStr);
                        arg.print(contextItems.printPolicy, os, true);
                        os.flush();
                        out += argStr;
                        break;
                    }}
                    out += ", ";
                }
                out  = TrimRightIf(out, ", ");
                out += "> ";
            }
            else
            {
                std::string exprStr;
                llvm::raw_string_ostream os(exprStr);
                requiresClause->printPretty(os, nullptr, contextItems.printPolicy);
                out += exprStr;
            }
        }
        return out;
    }
}