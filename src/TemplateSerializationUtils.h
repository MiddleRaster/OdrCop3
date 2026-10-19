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
        std::string out;
        if (const ASTTemplateArgumentListInfo* ArgsAsWritten = ctd->getTemplateArgsAsWritten())
        {
            llvm::ArrayRef<TemplateArgumentLoc> ArgsRef(ArgsAsWritten->getTemplateArgs(), ArgsAsWritten->NumTemplateArgs);
            llvm::raw_string_ostream os(out);
            PrintingPolicy policy{contextItems.printPolicy};
            policy.FullyQualifiedName     = true;
            policy.SuppressUnwrittenScope = true;
            clang::printTemplateArgumentList(os, ArgsRef, policy, nullptr);
            os.flush();
            return out;
        }
        return out;
    }

    template <auto SerializeDecl, auto SerializeType, auto SerializeExpr>
    inline std::string ConstructTemplateParameterList(const ContextItems& contextItems, const clang::TemplateParameterList* params)
    {
        auto AddParameterPackAndNameAndDefaultArgument = [](const ContextItems& contextItems, const auto* tp) -> std::string
                                                         {
                                                            std::string out;
                                                            if (tp->isParameterPack())
                                                                out += "...";
                                                            if (!tp->getName().empty())
                                                                out += tp->getName().str();
                                                            if (tp->hasDefaultArgument())
                                                            {
                                                                out += " = ";
                                                                const clang::TemplateArgument& argument = tp->getDefaultArgument().getArgument();
                                                                if (contextItems.serializationNeeds.AreAllFalse())
                                                                {   JustPrintIt:
                                                                    std::string defaultStr;
                                                                    llvm::raw_string_ostream defaultStream(defaultStr);
                                                                    argument.print(contextItems.printPolicy, defaultStream, false);
                                                                    out += defaultStr;
                                                                } else {
                                                                    switch (argument.getKind())
                                                                    {
                                                                    case clang::TemplateArgument::Expression: out += IndentBlock(SerializeExpr(contextItems, argument.getAsExpr()), LengthOfLastLine(out)); break;
                                                                    case clang::TemplateArgument::Type      : out += IndentBlock(SerializeType(contextItems, argument.getAsType()), LengthOfLastLine(out)); break;
                                                                    default: goto JustPrintIt;
                                                                    }
                                                                }
                                                            }
                                                            return TrimRightIf(out, " ");
                                                         };

        if (false == std::any_of(params->begin(), params->end(), [](const clang::NamedDecl* param) { return !param->isImplicit(); }))
            return {}; // all implicit parameters (e.g., an abbreviated function)
        
        std::string out = "template <";

        bool first = true;
        for (const clang::NamedDecl* param : *params)
        {
            if (first)
                first = false;
            else
                out += ", ";

            if (const auto* ttp = clang::dyn_cast<clang::TemplateTypeParmDecl>(param))
            {
                if (const clang::TypeConstraint* typeConstraint = ttp->getTypeConstraint())
                {
                    llvm::raw_string_ostream stream(out);
                    typeConstraint->print(stream, contextItems.printPolicy);    // as written: Cx, ns::Cx, or Same<int>
                    out += " ";
                } else
                    out += ttp->wasDeclaredWithTypename() ? "typename " : "class ";
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, ttp);
                out  = TrimRightIf(out, " ");
                continue;
            }
            if (const auto* nttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
            {   // NTTP where type is a struct (new to C++20)
                if (contextItems.serializationNeeds.AreAllFalse())
                {
                    std::string declStr;
                    llvm::raw_string_ostream declStream(declStr);
                    nttp->getType().print(declStream, contextItems.printPolicy);
                    out += declStr + " ";
                }
                else
                {
                    QualType nttpQT = nttp->getType().getCanonicalType();
                    out += IndentBlock(SerializeType(contextItems, nttpQT), LengthOfLastLine(out));
                    out  = TrimRightIf(out, " ");
                    out += SnugUpPointersAndReferences(out);
                }
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, nttp);
                continue;
            }
            if (const auto* ttp2 = clang::dyn_cast<clang::TemplateTemplateParmDecl>(param))
            {
                out += ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeExpr>(contextItems, ttp2->getTemplateParameters());
                out += "class ";
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, ttp2);
                continue;
            }
        }
        out += "> ";

        if (const clang::Expr* requiresClause = params->getRequiresClause())
        {
            out += "requires ";
            out += TrimRightIf(IndentBlock(SerializeExpr(contextItems, requiresClause), LengthOfLastLine(out)), " ");
            out += " ";
        }
        return out;
    }

    template <auto SerializeDecl, auto SerializeType, auto SerializeExpr>
    inline std::string GetTemplateHeader(const ContextItems& contextItems, const auto* templateParameterList)
    {
        if (!contextItems.serializationNeeds.AreAllFalse())
            return IndentBlock(ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeExpr>(contextItems, templateParameterList), 0);

        std::string                 templatePrefix;
        llvm::raw_string_ostream os(templatePrefix);

        clang::PrintingPolicy policy{contextItems.printPolicy};
        policy.FullyQualifiedName     = true;
        policy.SuppressUnwrittenScope = true;
        templateParameterList->print(os, contextItems.context, policy);
        os.flush();
        return templatePrefix;
    }
}